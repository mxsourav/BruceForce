const BUILD_TIMESTAMP = "2026-04-05T01:00:00+05:30";

const TARGET_LIBRARY = [
  { ssid: "CampusLab-5G", bssid: "B4:9D:16:4A:22:70", channel: 36, baseRssi: -42, phase: 0.2, drift: 2.8 },
  { ssid: "IoT-TestBed", bssid: "A0:13:EF:77:92:10", channel: 6, baseRssi: -58, phase: 1.1, drift: 3.1 },
  { ssid: "Guest_Network", bssid: "8C:3B:AD:12:DE:11", channel: 11, baseRssi: -64, phase: 2.1, drift: 2.2 },
  { ssid: "Ops_AP_2", bssid: "4E:F1:77:AD:65:91", channel: 1, baseRssi: -49, phase: 0.8, drift: 2.6 },
  { ssid: "ProbeMesh", bssid: "10:AC:BE:43:AA:0F", channel: 9, baseRssi: -67, phase: 1.8, drift: 1.9 },
  { ssid: "Workshop_Node", bssid: "C2:FF:EE:00:7A:19", channel: 13, baseRssi: -46, phase: 2.6, drift: 2.4 }
];

const PentestApp = {
  scanning: false,
  attacking: false,
  packetsPerSec: 0,
  selectedTarget: null,
  scanIntensity: 8,
  scanProgress: 0,
  beaconPpsSetting: 280,
  deauthActive: false,
  beaconActive: false,
  deploying: false,
  socket: null,
  socketMode: "offline",
  socketRoute: "ws://192.168.4.1/ws",
  simulationPhase: 0,
  lastSweepAt: null,
  chart: null,
  chartLabels: Array.from({ length: 24 }, () => ""),
  chartPackets: Array.from({ length: 24 }, () => 0),
  chartIntensity: Array.from({ length: 24 }, () => 0),
  targets: []
};

const els = {
  navLinks: document.querySelectorAll(".nav-link"),
  socketStatusChip: document.getElementById("socketStatusChip"),
  startSocketBtn: document.getElementById("startSocketBtn"),
  reconnectButton: document.getElementById("reconnectButton"),
  scanButton: document.getElementById("scanButton"),
  transportState: document.getElementById("transportState"),
  selectedTargetDisplay: document.getElementById("selectedTargetDisplay"),
  packetRateDisplay: document.getElementById("packetRateDisplay"),
  scanStateDisplay: document.getElementById("scanStateDisplay"),
  modeBadge: document.getElementById("modeBadge"),
  scanProgressLabel: document.getElementById("scanProgressLabel"),
  scanProgressFill: document.getElementById("scanProgressFill"),
  lastSweepDisplay: document.getElementById("lastSweepDisplay"),
  targetCountDisplay: document.getElementById("targetCountDisplay"),
  tableBadge: document.getElementById("tableBadge"),
  meterRing: document.getElementById("meterRing"),
  meterValue: document.getElementById("meterValue"),
  meterLabel: document.getElementById("meterLabel"),
  meterStateBadge: document.getElementById("meterStateBadge"),
  scanIntensityValue: document.getElementById("scanIntensityValue"),
  attackOutputValue: document.getElementById("attackOutputValue"),
  targetLockValue: document.getElementById("targetLockValue"),
  targetTableBody: document.getElementById("targetTableBody"),
  attackingBadge: document.getElementById("attackingBadge"),
  deauthCard: document.getElementById("deauthCard"),
  deauthToggle: document.getElementById("deauthToggle"),
  deauthLed: document.getElementById("deauthLed"),
  beaconSlider: document.getElementById("beaconSlider"),
  beaconRateDisplay: document.getElementById("beaconRateDisplay"),
  beaconButton: document.getElementById("beaconButton"),
  targetSsidInput: document.getElementById("targetSsidInput"),
  deployButton: document.getElementById("deployButton"),
  deploymentState: document.getElementById("deploymentState"),
  socketRouteInput: document.getElementById("socketRouteInput"),
  routeModeDisplay: document.getElementById("routeModeDisplay"),
  consoleWindow: document.getElementById("consoleWindow"),
  clearConsoleButton: document.getElementById("clearConsoleButton"),
  footerRoute: document.getElementById("footerRoute"),
  footerTarget: document.getElementById("footerTarget"),
  buildAge: document.getElementById("buildAge"),
  yearNow: document.getElementById("yearNow"),
  chartCanvas: document.getElementById("activityChart")
};

function init() {
  PentestApp.targets = TARGET_LIBRARY.map((target) => ({ ...target, rssi: target.baseRssi }));

  bindNav();
  bindButtons();
  bindControls();
  bindPressFeedback();
  initChart();
  refreshTargets();
  renderTargetTable();
  updateBeaconRateDisplay();
  updateTransportUi();
  updateStatusUi();
  updateActivityMeter();
  updateBuildAge();
  seedConsole();

  window.setInterval(updateBuildAge, 1000);
  window.setInterval(runTelemetryStep, 250);
  window.setInterval(pushChartPoint, 500);
  window.setInterval(() => {
    refreshTargets();
    renderTargetTable();
    updateStatusUi();
  }, 1200);
}

function bindNav() {
  els.navLinks.forEach((link) => {
    link.addEventListener("click", () => {
      els.navLinks.forEach((node) => node.classList.remove("active"));
      link.classList.add("active");
    });
  });
}

function bindButtons() {
  els.startSocketBtn.addEventListener("click", () => startSocket(false));
  els.reconnectButton.addEventListener("click", () => startSocket(true));
  els.scanButton.addEventListener("click", startScan);
  els.beaconButton.addEventListener("click", toggleBeacon);
  els.deployButton.addEventListener("click", deployTarget);
  els.clearConsoleButton.addEventListener("click", () => {
    els.consoleWindow.innerHTML = "";
    logToConsole("Activity feed cleared.", "normal");
  });
}

function bindControls() {
  els.deauthToggle.addEventListener("change", handleDeauthToggle);

  els.beaconSlider.addEventListener("input", () => {
    PentestApp.beaconPpsSetting = Number(els.beaconSlider.value);
    updateBeaconRateDisplay();
    updateStatusUi();

    if (PentestApp.beaconActive) {
      sendCommand("beacon.updateRate", { packetsPerSec: PentestApp.beaconPpsSetting });
    }
  });

  els.socketRouteInput.addEventListener("change", () => {
    PentestApp.socketRoute = els.socketRouteInput.value.trim() || "ws://192.168.4.1/ws";
    els.footerRoute.textContent = PentestApp.socketRoute;
  });

  els.targetSsidInput.addEventListener("input", updateStatusUi);
}

function bindPressFeedback() {
  document.querySelectorAll(".haptic-btn, .table-button").forEach((button) => {
    button.addEventListener("pointerdown", () => {
      button.classList.add("is-pressed");
      window.setTimeout(() => button.classList.remove("is-pressed"), 160);
    });
  });
}

function initChart() {
  PentestApp.chart = new Chart(els.chartCanvas, {
    type: "line",
    data: {
      labels: PentestApp.chartLabels,
      datasets: [
        {
          label: "Packets / Sec",
          data: PentestApp.chartPackets,
          borderColor: "#5ca6ff",
          backgroundColor: "rgba(92, 166, 255, 0.18)",
          fill: true,
          borderWidth: 2.6,
          pointRadius: 0,
          tension: 0.36
        },
        {
          label: "Scan Intensity",
          data: PentestApp.chartIntensity,
          borderColor: "#ff992f",
          borderWidth: 2,
          pointRadius: 0,
          tension: 0.34,
          borderDash: [6, 5],
          yAxisID: "y1"
        }
      ]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: {
        duration: 320,
        easing: "easeOutQuart"
      },
      plugins: {
        legend: { display: false },
        tooltip: {
          backgroundColor: "rgba(17, 19, 24, 0.96)",
          borderColor: "rgba(255, 255, 255, 0.08)",
          borderWidth: 1,
          titleColor: "#f3f5f8",
          bodyColor: "#d9deea"
        }
      },
      scales: {
        x: {
          grid: {
            color: "rgba(255, 255, 255, 0.04)"
          },
          ticks: {
            color: "#8c96a5",
            maxTicksLimit: 6
          }
        },
        y: {
          beginAtZero: true,
          suggestedMax: 700,
          grid: {
            color: "rgba(255, 255, 255, 0.05)"
          },
          ticks: {
            color: "#8c96a5"
          }
        },
        y1: {
          beginAtZero: true,
          min: 0,
          max: 100,
          position: "right",
          grid: {
            drawOnChartArea: false
          },
          ticks: {
            color: "#c8935e"
          }
        }
      }
    }
  });
}

function startSocket(forceReconnect) {
  PentestApp.socketRoute = els.socketRouteInput.value.trim() || "ws://192.168.4.1/ws";
  els.socketRouteInput.value = PentestApp.socketRoute;
  els.footerRoute.textContent = PentestApp.socketRoute;

  if (forceReconnect) {
    closeTransport();
  } else if (["connecting", "connected", "mock"].includes(PentestApp.socketMode)) {
    logToConsole("Transport already initialized. Use reconnect if you want to reset the route.", "warning");
    return;
  }

  PentestApp.socketMode = "connecting";
  updateTransportUi();
  logToConsole(`Opening WebSocket transport at ${PentestApp.socketRoute}.`, "normal");

  if (typeof WebSocket === "undefined") {
    attachMockSocket(PentestApp.socketRoute);
    return;
  }

  let settled = false;

  try {
    const socket = new WebSocket(PentestApp.socketRoute);
    PentestApp.socket = socket;

    const fallbackTimer = window.setTimeout(() => {
      if (settled) return;
      settled = true;
      try {
        socket.close();
      } catch (error) {
        void error;
      }
      attachMockSocket(PentestApp.socketRoute);
    }, 1200);

    socket.addEventListener("open", () => {
      if (settled) return;
      settled = true;
      window.clearTimeout(fallbackTimer);
      PentestApp.socketMode = "connected";
      updateTransportUi();
      logToConsole(`Live transport established on ${PentestApp.socketRoute}.`, "normal");
    });

    socket.addEventListener("message", (event) => {
      logToConsole(`ESP32: ${event.data}`, "normal");
    });

    socket.addEventListener("error", () => {
      if (settled) return;
      settled = true;
      window.clearTimeout(fallbackTimer);
      attachMockSocket(PentestApp.socketRoute);
    });

    socket.addEventListener("close", () => {
      if (PentestApp.socketMode === "connected") {
        PentestApp.socketMode = "offline";
        PentestApp.socket = null;
        updateTransportUi();
        logToConsole("Live transport closed.", "warning");
      }
    });
  } catch (error) {
    logToConsole(`WebSocket bootstrap failed: ${error.message}`, "critical");
    attachMockSocket(PentestApp.socketRoute);
  }
}

function closeTransport() {
  if (PentestApp.socket && typeof PentestApp.socket.close === "function") {
    try {
      PentestApp.socket.close();
    } catch (error) {
      void error;
    }
  }

  PentestApp.socket = null;
  PentestApp.socketMode = "offline";
  updateTransportUi();
}

function attachMockSocket(route) {
  PentestApp.socket = {
    readyState: 1,
    mock: true,
    send(payload) {
      console.log("Mock transport payload:", payload);
    },
    close() {
      console.log("Mock transport closed");
    }
  };

  PentestApp.socketMode = "mock";
  updateTransportUi();
  logToConsole(`Route unavailable. Switched to simulated ESP32 transport at ${route}.`, "warning");
}

function sendCommand(action, params = {}) {
  const payload = {
    action,
    params,
    issuedAt: new Date().toISOString()
  };

  if (PentestApp.socketMode === "connected" && PentestApp.socket?.readyState === 1) {
    PentestApp.socket.send(JSON.stringify(payload));
  } else if (PentestApp.socketMode === "mock" && PentestApp.socket?.send) {
    PentestApp.socket.send(JSON.stringify(payload));
  } else {
    logToConsole("No active transport. Command routed through local simulator.", "warning");
  }

  logToConsole(`Command sent: ${action}`, "normal");
  simulateDeviceResponse(action, params);
}

function simulateDeviceResponse(action, params) {
  const targetName = params.target || params.ssid || (PentestApp.selectedTarget ? PentestApp.selectedTarget.ssid : "pending target");

  if (action === "scan.start") {
    scheduleResponse(220, "ESP32 acknowledged scan request.", "normal");
    scheduleResponse(980, "Survey mode active. Sampling nearby radios.", "normal");
    return;
  }

  if (action === "deauth.toggle") {
    if (params.active) {
      scheduleResponse(180, `Deauth armed for ${targetName}.`, "warning");
    } else {
      scheduleResponse(180, "Deauth module returned to standby.", "normal");
    }
    return;
  }

  if (action === "beacon.start") {
    scheduleResponse(220, `Beacon emitter ramping to ${params.packetsPerSec} packets per second.`, "warning");
    return;
  }

  if (action === "beacon.stop") {
    scheduleResponse(220, "Beacon emitter returned to standby.", "normal");
    return;
  }

  if (action === "beacon.updateRate") {
    scheduleResponse(140, `Beacon rate updated to ${params.packetsPerSec} packets per second.`, "normal");
    return;
  }

  if (action === "deploy.clone") {
    scheduleResponse(260, `Clone profile staged for ${targetName}.`, "warning");
    scheduleResponse(2300, `Deployment placeholder completed for ${targetName}.`, "normal");
  }
}

function scheduleResponse(delay, message, type) {
  window.setTimeout(() => logToConsole(message, type), delay);
}

function startScan() {
  if (PentestApp.scanning) {
    logToConsole("A scan is already running.", "warning");
    return;
  }

  PentestApp.scanning = true;
  PentestApp.scanProgress = 0;
  updateStatusUi();
  logToConsole("Network sweep started. Collecting nearby SSIDs and signal values.", "normal");
  sendCommand("scan.start", { passive: true });

  const progressTimer = window.setInterval(() => {
    PentestApp.scanProgress = Math.min(PentestApp.scanProgress + 8, 100);
    updateStatusUi();

    if ([24, 56, 88].includes(PentestApp.scanProgress)) {
      refreshTargets();
      renderTargetTable();
      logToConsole(`Scan update: ${PentestApp.targets.length} targets currently sampled.`, "normal");
    }

    if (PentestApp.scanProgress >= 100) {
      window.clearInterval(progressTimer);
      PentestApp.scanning = false;
      PentestApp.lastSweepAt = Date.now();
      refreshTargets();
      renderTargetTable();
      updateStatusUi();
      logToConsole(`Scan complete. ${PentestApp.targets.length} targets available in the table.`, "normal");

      window.setTimeout(() => {
        PentestApp.scanProgress = 0;
        updateStatusUi();
      }, 900);
    }
  }, 260);
}

function handleDeauthToggle() {
  PentestApp.deauthActive = els.deauthToggle.checked;
  updateAttackState();

  if (PentestApp.deauthActive && !PentestApp.selectedTarget && !els.targetSsidInput.value.trim()) {
    logToConsole("Deauth armed without a selected target. Choose a target before transmitting.", "warning");
  }

  sendCommand("deauth.toggle", {
    active: PentestApp.deauthActive,
    target: getTargetName()
  });

  updateStatusUi();
}

function toggleBeacon() {
  PentestApp.beaconActive = !PentestApp.beaconActive;
  updateAttackState();

  if (PentestApp.beaconActive) {
    sendCommand("beacon.start", { packetsPerSec: PentestApp.beaconPpsSetting });
  } else {
    sendCommand("beacon.stop", { packetsPerSec: PentestApp.beaconPpsSetting });
  }

  updateStatusUi();
}

function deployTarget() {
  const ssid = els.targetSsidInput.value.trim();

  if (!ssid) {
    logToConsole("Enter or select a target SSID before deployment.", "critical");
    return;
  }

  PentestApp.deploying = true;
  updateAttackState();
  updateStatusUi();
  sendCommand("deploy.clone", { ssid });
  logToConsole(`Deployment started for ${ssid}.`, "warning");

  window.setTimeout(() => {
    PentestApp.deploying = false;
    updateAttackState();
    updateStatusUi();
  }, 2600);
}

function refreshTargets() {
  PentestApp.targets = TARGET_LIBRARY.map((target, index) => {
    const driftWave = Math.sin(PentestApp.simulationPhase * 0.95 + target.phase) * target.drift;
    const scanWave = PentestApp.scanning ? Math.cos(PentestApp.simulationPhase * 1.5 + index) * 1.8 : 0;
    const cycleOffset = ((index + Math.floor(PentestApp.simulationPhase * 2)) % 3) - 1;
    const rssi = clamp(Math.round(target.baseRssi + driftWave + scanWave + cycleOffset), -82, -34);

    return {
      ...target,
      rssi
    };
  }).sort((left, right) => right.rssi - left.rssi);

  if (PentestApp.selectedTarget) {
    PentestApp.selectedTarget = PentestApp.targets.find((target) => target.bssid === PentestApp.selectedTarget.bssid) || PentestApp.selectedTarget;
  }
}

function renderTargetTable() {
  els.targetTableBody.innerHTML = "";

  PentestApp.targets.forEach((target) => {
    const row = document.createElement("tr");
    if (PentestApp.selectedTarget?.bssid === target.bssid) {
      row.classList.add("is-selected");
    }

    row.innerHTML = `
      <td>${target.ssid}</td>
      <td>${target.bssid}</td>
      <td>${target.channel}</td>
      <td><span class="rssi-pill ${signalClassForRssi(target.rssi)}">${target.rssi} dBm</span></td>
      <td><button class="hardware-button table-button" type="button">Select Target</button></td>
    `;

    const selectButton = row.querySelector("button");
    selectButton.addEventListener("click", () => selectTarget(target.bssid));
    selectButton.addEventListener("pointerdown", () => {
      selectButton.classList.add("is-pressed");
      window.setTimeout(() => selectButton.classList.remove("is-pressed"), 160);
    });
    els.targetTableBody.appendChild(row);
  });

  els.targetCountDisplay.textContent = String(PentestApp.targets.length);
  els.tableBadge.textContent = `${PentestApp.targets.length} targets loaded`;
}

function selectTarget(bssid) {
  PentestApp.selectedTarget = PentestApp.targets.find((target) => target.bssid === bssid) || null;

  if (!PentestApp.selectedTarget) {
    return;
  }

  els.targetSsidInput.value = PentestApp.selectedTarget.ssid;
  updateStatusUi();
  renderTargetTable();
  logToConsole(`Target selected: ${PentestApp.selectedTarget.ssid} on channel ${PentestApp.selectedTarget.channel}.`, "normal");
}

function runTelemetryStep() {
  PentestApp.simulationPhase += 0.18;

  const idleLoad = 6 + 2.8 * wave(0.7);
  const scanLoad = PentestApp.scanning ? 82 + 16 * wave(1.05) + 10 * wave(1.8, 0.7) : 0;
  const deauthLoad = PentestApp.deauthActive ? 136 + 18 * wave(1.35, 0.6) : 0;
  const beaconLoad = PentestApp.beaconActive ? PentestApp.beaconPpsSetting * (0.56 + 0.06 * wave(0.82, 0.4)) : 0;
  const deployLoad = PentestApp.deploying ? 58 + 8 * wave(1.1, 1.2) : 0;
  const desiredPackets = Math.max(0, idleLoad + scanLoad + deauthLoad + beaconLoad + deployLoad);

  const intensityBase = 8 + 3 * wave(0.55);
  const scanIntensity = PentestApp.scanning ? 44 + 14 * wave(0.9, 0.2) + 8 * wave(1.4, 0.5) : 0;
  const beaconIntensity = PentestApp.beaconActive ? Math.min(34, PentestApp.beaconPpsSetting / 32) : 0;
  const deauthIntensity = PentestApp.deauthActive ? 20 : 0;
  const deployIntensity = PentestApp.deploying ? 14 : 0;
  const desiredIntensity = clamp(intensityBase + scanIntensity + beaconIntensity + deauthIntensity + deployIntensity, 0, 100);

  PentestApp.packetsPerSec = smoothValue(PentestApp.packetsPerSec, desiredPackets, PentestApp.attacking ? 0.28 : 0.2);
  PentestApp.scanIntensity = smoothValue(PentestApp.scanIntensity, desiredIntensity, 0.18);

  updateStatusUi();
  updateActivityMeter();
}

function pushChartPoint() {
  const stamp = new Date().toLocaleTimeString([], {
    minute: "2-digit",
    second: "2-digit"
  });

  PentestApp.chartLabels.shift();
  PentestApp.chartLabels.push(stamp);
  PentestApp.chartPackets.shift();
  PentestApp.chartPackets.push(Math.round(PentestApp.packetsPerSec));
  PentestApp.chartIntensity.shift();
  PentestApp.chartIntensity.push(Math.round(PentestApp.scanIntensity));

  PentestApp.chart.data.labels = [...PentestApp.chartLabels];
  PentestApp.chart.data.datasets[0].data = [...PentestApp.chartPackets];
  PentestApp.chart.data.datasets[1].data = [...PentestApp.chartIntensity];
  PentestApp.chart.update();
}

function updateAttackState() {
  PentestApp.attacking = PentestApp.deauthActive || PentestApp.beaconActive || PentestApp.deploying;
}

function updateTransportUi() {
  let chip = "Socket Offline";
  let transport = "Offline";
  let routeMode = "Mock Ready";

  if (PentestApp.socketMode === "connecting") {
    chip = "Socket Connecting";
    transport = "Connecting";
    routeMode = "Connecting";
  } else if (PentestApp.socketMode === "connected") {
    chip = "Socket Live";
    transport = "Live";
    routeMode = "Live Socket";
  } else if (PentestApp.socketMode === "mock") {
    chip = "Socket Mock";
    transport = "Mock";
    routeMode = "Mock Transport";
  }

  els.socketStatusChip.textContent = chip;
  els.transportState.textContent = transport;
  els.routeModeDisplay.textContent = routeMode;
  els.footerRoute.textContent = PentestApp.socketRoute;
}

function updateStatusUi() {
  const packets = Math.round(PentestApp.packetsPerSec);
  const targetName = getTargetName();
  const scanState = PentestApp.scanning ? "Running" : "Idle";
  const mode = PentestApp.scanning ? "Scanning" : PentestApp.attacking ? "Active" : "Idle";

  els.packetRateDisplay.textContent = String(packets);
  els.selectedTargetDisplay.textContent = targetName;
  els.scanStateDisplay.textContent = scanState;
  els.modeBadge.textContent = mode;
  els.scanProgressFill.style.width = `${PentestApp.scanProgress}%`;
  els.scanProgressLabel.textContent = PentestApp.scanning ? `${PentestApp.scanProgress}%` : PentestApp.lastSweepAt ? "Complete" : "Idle";
  els.lastSweepDisplay.textContent = PentestApp.lastSweepAt ? formatClock(PentestApp.lastSweepAt) : "Waiting";
  els.beaconButton.textContent = PentestApp.beaconActive ? "Stop Beacon" : "Start Beacon";
  els.attackingBadge.textContent = PentestApp.attacking ? "Attack Active" : "Standby";
  els.deploymentState.textContent = PentestApp.deploying ? "Deploying" : "Ready";
  els.attackOutputValue.textContent = describeAttackOutput();
  els.targetLockValue.textContent = targetName;
  els.footerTarget.textContent = targetName.toUpperCase();

  els.deauthCard.classList.toggle("is-active", PentestApp.deauthActive);
  els.deauthCard.classList.toggle("is-armed", PentestApp.deauthActive);
}

function updateActivityMeter() {
  const level = clamp(Math.round(PentestApp.scanIntensity + PentestApp.packetsPerSec / 12), 0, 100);
  const meterState = level >= 74 ? "High" : level >= 28 ? "Active" : "Idle";
  const meterColor = level >= 74 ? "#ff6450" : level >= 28 ? "#ff992f" : "#5ca6ff";
  const angle = Math.max(24, Math.round((level / 100) * 300));

  els.meterRing.style.setProperty("--meter-angle", `${angle}deg`);
  els.meterRing.style.setProperty("--meter-color", meterColor);
  els.meterValue.textContent = `${level}%`;
  els.meterLabel.textContent = meterState;
  els.meterStateBadge.textContent = meterState;
  els.scanIntensityValue.textContent = `${Math.round(PentestApp.scanIntensity)}%`;
}

function updateBeaconRateDisplay() {
  els.beaconRateDisplay.textContent = `${PentestApp.beaconPpsSetting} PPS`;
}

function logToConsole(message, type = "normal") {
  const entry = document.createElement("div");
  entry.className = `console-line ${type}`;

  const time = document.createElement("div");
  time.className = "console-time";
  time.textContent = new Date().toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });

  const payload = document.createElement("div");
  payload.className = "console-message";
  payload.textContent = message;

  entry.append(time, payload);
  els.consoleWindow.appendChild(entry);
  els.consoleWindow.scrollTop = els.consoleWindow.scrollHeight;
}

function seedConsole() {
  logToConsole("DeautherNet UI initialized.", "normal");
  logToConsole("WebSocket transport is placeholder-ready and will fall back to mock responses when the route is unavailable.", "warning");
  logToConsole("Activity simulation is deterministic and tied to real UI state changes.", "normal");
}

function updateBuildAge() {
  els.yearNow.textContent = String(new Date().getFullYear());
  const releaseTime = new Date(BUILD_TIMESTAMP).getTime();
  const elapsed = Math.max(0, Date.now() - releaseTime);
  const totalSeconds = Math.floor(elapsed / 1000);
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  els.buildAge.textContent = `${pad(hours)}h ${pad(minutes)}m ${pad(seconds)}s passed`;
}

function getTargetName() {
  if (PentestApp.selectedTarget) {
    return PentestApp.selectedTarget.ssid;
  }

  const draft = els.targetSsidInput.value.trim();
  return draft || "None";
}

function describeAttackOutput() {
  if (PentestApp.deauthActive && PentestApp.beaconActive) {
    return "Mixed Load";
  }

  if (PentestApp.deauthActive) {
    return "Deauth";
  }

  if (PentestApp.beaconActive) {
    return "Beacon";
  }

  if (PentestApp.deploying) {
    return "Deploying";
  }

  return "Standby";
}

function formatClock(timestamp) {
  return new Date(timestamp).toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });
}

function pad(value) {
  return String(value).padStart(2, "0");
}

function wave(speed, offset = 0) {
  return Math.sin(PentestApp.simulationPhase * speed + offset);
}

function smoothValue(current, target, factor) {
  return current + (target - current) * factor;
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function signalClassForRssi(rssi) {
  if (rssi >= -50) {
    return "signal-strong";
  }

  if (rssi >= -65) {
    return "signal-mid";
  }

  return "signal-weak";
}

window.addEventListener("load", init);
