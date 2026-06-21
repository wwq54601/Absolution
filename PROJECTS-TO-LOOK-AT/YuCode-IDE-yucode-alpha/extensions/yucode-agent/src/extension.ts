import * as vscode from "vscode";
import * as cp from "child_process";
import * as path from "path";
import * as fs from "fs";

const AGENT_URL = "http://127.0.0.1:8765";

let runtimeProcess: cp.ChildProcess | undefined;
let reindexTimer: NodeJS.Timeout | undefined;
let lastVerifyOutput = "";
let resultBuffer = "";
const proposedDocuments = new Map<string, string>();
let fileIndexTimer: NodeJS.Timeout | undefined;
let detectedBackend: "cuda13" | "cuda12" | "vulkan" | "cpu" = "cpu";
let detectedGpuName = "";
const pendingFileUpdates = new Set<string>();
const pendingFileRemoves = new Set<string>();

type PendingChangeFile = {
  file_path: string;
  old_content: string;
  new_content: string;
  unified_diff?: string;
};

type PendingChange = {
  id: string;
  file_path: string;
  explanation: string;
  old_content: string;
  new_content: string;
  unified_diff?: string;
  files?: PendingChangeFile[];
};

type ChangesResponse = {
  changes?: PendingChange[];
};

type StatusResponse = {
  status?: string;
  name?: string;
  version?: string;
};

export async function activate(context: vscode.ExtensionContext) {

  const backendInfo = await detectBackend(context);
  detectedBackend = backendInfo.backend;
  detectedGpuName = backendInfo.gpuName;

  await writeEmbeddedConfigBeforeRuntimeStart(context);

  await ensureRuntimeStarted(context);

  const proposedProvider = vscode.workspace.registerTextDocumentContentProvider(
    "yucode-proposed",
    {
      provideTextDocumentContent(uri: vscode.Uri): string {
        return proposedDocuments.get(uri.toString()) ?? "";
      }
    }
  );

  const sidebarProvider = new YuCodeSidebarProvider(context);

  const watcher = vscode.workspace.createFileSystemWatcher("**/*");

watcher.onDidCreate(uri => scheduleFileUpdate(uri.fsPath));
watcher.onDidChange(uri => scheduleFileUpdate(uri.fsPath));
watcher.onDidDelete(uri => scheduleFileRemove(uri.fsPath));

context.subscriptions.push(watcher);

  context.subscriptions.push(
    proposedProvider,
    vscode.window.registerWebviewViewProvider(
      "yucode.agentView",
      sidebarProvider
    ),
    vscode.commands.registerCommand("yucode.openAgent", async () => {
      await vscode.commands.executeCommand("workbench.view.extension.yucode");
    })
  );

  setTimeout(async () => {
  await vscode.commands.executeCommand("workbench.action.moveSideBarRight");
  await vscode.commands.executeCommand("workbench.view.extension.yucode");
}, 800);
}

export function deactivate() {
  if (reindexTimer) {
    clearTimeout(reindexTimer);
    reindexTimer = undefined;
  }

  if (fileIndexTimer) {
    clearTimeout(fileIndexTimer);
    fileIndexTimer = undefined;
  }

  if (runtimeProcess) {
    runtimeProcess.kill();
    runtimeProcess = undefined;
  }
}

class YuCodeSidebarProvider implements vscode.WebviewViewProvider {
  private view?: vscode.WebviewView;

  constructor(private readonly context: vscode.ExtensionContext) {}

  resolveWebviewView(webviewView: vscode.WebviewView) {
    this.view = webviewView;

    webviewView.webview.options = {
  enableScripts: true
};

    webviewView.webview.html = getWebviewHtml();

    webviewView.webview.onDidReceiveMessage(async (message) => {
      try {
        if (message.type === "checkStatus") {
          await this.checkStatus();
        }

        if (message.type === "openSettings") {
  const response = await fetch(`${AGENT_URL}/api/config`);
  const config = await response.json();

  this.post({
    type: "settingsData",
    config
  });
}

if (message.type === "modelStatus") {
  const response = await fetch(`${AGENT_URL}/api/model/status`);
  const result = await response.json();

  this.post({
    type: "modelStatusResult",
    result
  });
}

if (message.type === "testLlm") {
  const response = await fetch(`${AGENT_URL}/api/test-llm`, {
    method: "POST"
  });

  const result = await response.json();

  this.post({
    type: "testLlmResult",
    result
  });
}

if (message.type === "testEmbedding") {
  const response = await fetch(`${AGENT_URL}/api/test-embedding`, {
    method: "POST"
  });

  const result = await response.json();

  this.post({
    type: "testEmbeddingResult",
    result
  });
}

if (message.type === "embeddedRuntimeStatus") {
  const response = await fetch(`${AGENT_URL}/api/embedded-runtime/status`);
  const result = await response.json();

  this.post({
    type: "embeddedRuntimeStatusResult",
    result
  });
}

if (message.type === "indexStatus") {
  const response = await fetch(`${AGENT_URL}/api/index/status`);
  const result = await response.json();

  this.post({
    type: "indexStatusResult",
    result
  });
}

        if (message.type === "runAgent") {
          const workspace = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? "";
          const activeFile = vscode.window.activeTextEditor?.document.uri.fsPath ?? "";

          const editor = vscode.window.activeTextEditor;
const selectedText = editor
  ? editor.document.getText(editor.selection)
  : "";

          await this.runAgentStreaming({
  query: message.query,
  mode: message.mode ?? "auto",
  workspace_path: workspace,
  active_file: activeFile,
  selected_text: selectedText,
  extra_context: message.useLastVerifyOutput
    ? lastVerifyOutput
    : ""
});

await this.refreshChanges();
        }

        if (message.type === "reindex") {
  const workspace = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? "";

  const response = await fetch(`${AGENT_URL}/api/index`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({
      workspace_path: workspace
    })
  });

  const data = await response.json();

  this.post({
    type: "reindexResult",
    result: data
  });

  await this.refreshChanges();
}

if (message.type === "clearSession") {
  const result = await postJson(`${AGENT_URL}/api/session/clear`, {});
  this.post({
    type: "clearSessionResult",
    result
  });
}

        if (message.type === "refreshChanges") {
          await this.refreshChanges();
        }

        if (message.type === "applyChange") {
          const applyResult = await postJson(`${AGENT_URL}/api/changes/apply`, {
  id: message.id
});
const resultAny = applyResult as any;
lastVerifyOutput =
  JSON.stringify(resultAny.errors ?? [], null, 2)
  + "\n\n"
  + (resultAny.verify_output ?? "");

this.post({
  type: "applyResult",
  result: applyResult
});

vscode.window.showInformationMessage(`YuCode applied ${message.id}`);
await this.refreshChanges();
        }

        if (message.type === "rejectChange") {
          await postJson(`${AGENT_URL}/api/changes/reject`, {
            id: message.id
          });

          vscode.window.showInformationMessage(`YuCode rejected ${message.id}`);
          await this.refreshChanges();
        }

        if (message.type === "openFile") {
          const doc = await vscode.workspace.openTextDocument(message.filePath);
          await vscode.window.showTextDocument(doc, vscode.ViewColumn.One);
        }

        if (message.type === "saveSettings") {
  const response = await fetch(`${AGENT_URL}/api/config`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(message.config)
  });

  const result = await response.json();

  this.post({
    type: "settingsSaved",
    result
  });
}

        if (message.type === "openDiff") {
          const originalUri = vscode.Uri.file(message.filePath);

          const proposedUri = vscode.Uri.parse(
            `yucode-proposed:${encodeURIComponent(message.filePath)}?id=${encodeURIComponent(message.id)}`
          );

          proposedDocuments.set(proposedUri.toString(), message.newContent ?? "");

          await vscode.commands.executeCommand(
            "vscode.diff",
            originalUri,
            proposedUri,
            `YuCode Change: ${message.id}`
          );
        }
      } catch (error) {
        this.post({
          type: "error",
          error: String(error)
        });
      }
    });

    this.checkStatus();
    this.refreshChanges();
  }

  private post(message: unknown) {
    this.view?.webview.postMessage(message);
  }

  private async checkStatus() {
    try {
      const response = await fetch(`${AGENT_URL}/api/status`);
      const data = (await response.json()) as StatusResponse;

      this.post({
  type: "status",
  online: true,
  status: data,
  backend: detectedBackend,
  gpuName: detectedGpuName
});
    } catch (error) {
      this.post({
        type: "status",
        online: false,
        error: String(error)
      });
    }
  }

  private async runAgentStreaming(
  payload: any
) {
  resultBuffer = "";

  this.post({
    type: "agentStreamStart"
  });

  const response = await fetch(
    `${AGENT_URL}/api/agent/stream`,
    {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify(payload)
    }
  );

  if (!response.body) {
    throw new Error("Streaming body missing.");
  }

  const reader = response.body.getReader();
  const decoder = new TextDecoder();

  let pending = "";

  while (true) {
    const { done, value } =
      await reader.read();

    if (done) {
      break;
    }

    pending += decoder.decode(value, {
      stream: true
    });

    const events = pending.split("\n\n");
    pending = events.pop() ?? "";

    for (const rawEvent of events) {

      let eventType = "";
      let dataLine = "";

      for (const line of rawEvent.split("\n")) {

        if (line.startsWith("event:")) {
          eventType =
            line.substring(6).trim();
        }

        if (line.startsWith("data:")) {
          dataLine =
            line.substring(5).trim();
        }
      }

      if (!dataLine) {
        continue;
      }

      let data: any;

      try {
        data = JSON.parse(dataLine);
      } catch {
        continue;
      }

      if (eventType === "status") {

        this.post({
          type: "agentStreamStatus",
          message: data.message
        });

      } else if (eventType === "token") {

  let chunk = data.chunk ?? "";

  if (chunk === "created") {
    chunk = "";
  }

  resultBuffer += chunk;

  this.post({
    type: "agentStreamToken",
    chunk
  });

} else if (eventType === "done") {

  if (!data.message && resultBuffer.trim()) {
    data.message = resultBuffer.trim();
  }

  this.post({
    type: "agentResult",
    result: data
  });
}
    }
  }
}

  private async refreshChanges() {
    try {
      const response = await fetch(`${AGENT_URL}/api/changes`);
      const data = (await response.json()) as ChangesResponse;

      this.post({
        type: "changes",
        changes: data.changes ?? []
      });
    } catch (error) {
      this.post({
        type: "error",
        error: String(error)
      });
    }
  }
}

async function writeEmbeddedConfigBeforeRuntimeStart(
  context: vscode.ExtensionContext
) {
  const embeddedConfig = configureEmbeddedRuntime(context);

  const runtimeExe = findRuntimeExecutable(context);

  const configPaths: string[] = [
    path.join(context.extensionPath, "yucode.config.json")
  ];

  if (runtimeExe) {
    configPaths.push(
      path.join(path.dirname(runtimeExe), "yucode.config.json")
    );
  }

  for (const configPath of configPaths) {
    const dir = path.dirname(configPath);

    if (!fs.existsSync(dir)) {
      continue;
    }

    fs.writeFileSync(
      configPath,
      JSON.stringify(embeddedConfig, null, 2),
      "utf8"
    );

    console.log("[YuCode] Embedded config written:", configPath);
  }
}

async function postJson(url: string, body: unknown) {
  const response = await fetch(url, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(body)
  });

  return await response.json();
}

function scheduleReindex(changedPath?: string) {
  if (changedPath && shouldIgnorePath(changedPath)) {
    return;
  }

  if (reindexTimer) {
    clearTimeout(reindexTimer);
  }

  reindexTimer = setTimeout(async () => {
    await reindexWorkspace();
  }, 1500);
}

function scheduleFileUpdate(filePath: string) {
  if (shouldIgnorePath(filePath)) {
    return;
  }

  pendingFileUpdates.add(filePath);
  pendingFileRemoves.delete(filePath);

  scheduleFileIndexFlush();
}

function scheduleFileRemove(filePath: string) {
  if (shouldIgnorePath(filePath)) {
    return;
  }

  pendingFileRemoves.add(filePath);
  pendingFileUpdates.delete(filePath);

  scheduleFileIndexFlush();
}

function scheduleFileIndexFlush() {
  if (fileIndexTimer) {
    clearTimeout(fileIndexTimer);
  }

  fileIndexTimer = setTimeout(async () => {
    await flushFileIndexUpdates();
  }, 800);
}

async function flushFileIndexUpdates() {
  const updates = Array.from(pendingFileUpdates);
  const removes = Array.from(pendingFileRemoves);

  pendingFileUpdates.clear();
  pendingFileRemoves.clear();

  for (const filePath of updates) {
    await updateIndexFile(filePath);
  }

  for (const filePath of removes) {
    await removeIndexFile(filePath);
  }
}

async function updateIndexFile(filePath: string) {
  try {
    await fetch(`${AGENT_URL}/api/index/file`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        file_path: filePath
      })
    });
  } catch (error) {
    console.error("[YuCode] update index file failed:", error);
  }
}

async function removeIndexFile(filePath: string) {
  try {
    await fetch(`${AGENT_URL}/api/index/file/remove`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        file_path: filePath
      })
    });
  } catch (error) {
    console.error("[YuCode] remove index file failed:", error);
  }
}

function shouldIgnorePath(filePath: string): boolean {
  const normalized = filePath.replace(/\\/g, "/").toLowerCase();

  const ignored = [
    "/.git/",
    "/node_modules/",
    "/dist/",
    "/build/",
    "/out/",
    "/target/",
    "/.next/",
    "/.nuxt/",
    "/.venv/",
    "/venv/",
    "/agent-runtime/build/",
    "/extensions/yucode-agent/node_modules/",
    ".yucode.bak"
  ];

  return ignored.some(part => normalized.includes(part));
}

async function reindexWorkspace() {
  const workspace = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? "";

  if (!workspace) {
    return;
  }

  try {
    await fetch(`${AGENT_URL}/api/index`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        workspace_path: workspace
      })
    });

    console.log("[YuCode] Workspace reindexed.");
  } catch (error) {
    console.error("[YuCode] Auto reindex failed:", error);
  }
}

async function detectBackend(context: vscode.ExtensionContext): Promise<{
  backend: "cuda13" | "cuda12" | "vulkan" | "cpu";
  gpuName: string;
}> {
  return new Promise(resolve => {
    cp.exec(
      `powershell -NoProfile -Command "Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name"`,
      (error, stdout) => {
        const gpuName = stdout.trim();
        const lower = gpuName.toLowerCase();

        const runtimeDir = path.join(context.extensionPath, "runtime");

        const cuda13 = path.join(runtimeDir, "cuda13", "llama-server.exe");
const cuda12 = path.join(runtimeDir, "cuda12", "llama-server.exe");
const vulkan = path.join(runtimeDir, "vulkan", "llama-server.exe");
const cpu = path.join(runtimeDir, "cpu", "llama-server.exe");

        if (lower.includes("nvidia")) {
          if (fs.existsSync(cuda13)) {
            resolve({ backend: "cuda13", gpuName });
            return;
          }

          if (fs.existsSync(cuda12)) {
            resolve({ backend: "cuda12", gpuName });
            return;
          }
        }

        if (
  lower.includes("amd") ||
  lower.includes("radeon")
) {
  if (fs.existsSync(vulkan)) {
    resolve({ backend: "vulkan", gpuName });
    return;
  }
}

if (fs.existsSync(cpu)) {
  resolve({ backend: "cpu", gpuName });
  return;
}

        resolve({ backend: "cpu", gpuName });
      }
    );
  });
}

function getLlamaServerPath(
  context: vscode.ExtensionContext
): string {
  return path.join(
    context.extensionPath,
    "runtime",
    detectedBackend,
    "llama-server.exe"
  );
}

type EmbeddedRuntimeConfig = {
  llm_provider: string;
  llm_base_url: string;
  llm_model: string;
  embedded_runtime_enabled: boolean;
  embedded_server_path: string;
  embedded_model_path: string;
  embedded_runtime_port: number;
  embedded_context_size: number;
  embedded_gpu_layers: number;
};

function configureEmbeddedRuntime(
  context: vscode.ExtensionContext
): EmbeddedRuntimeConfig {
  const serverPath = path.join(
    context.extensionPath,
    "runtime",
    detectedBackend,
    "llama-server.exe"
  );

  const modelPath = path.join(
    context.extensionPath,
    "runtime",
    "models",
    "code-model.gguf"
  );

  return {
    llm_provider: "yucode-local",
    llm_base_url: "http://127.0.0.1:11435/v1",
    llm_model: "code-model",

    embedded_runtime_enabled: true,
    embedded_server_path: serverPath,
    embedded_model_path: modelPath,
    embedded_runtime_port: 11435,
embedded_context_size: 32768,
embedded_gpu_layers: detectedBackend === "cpu" ? 0 : 999
  };
}

async function ensureRuntimeStarted(context: vscode.ExtensionContext) {
  const alreadyOnline = await isRuntimeOnline();

  if (alreadyOnline) {
    return;
  }

  const exePath = findRuntimeExecutable(context);

  console.log("[YuCode] exePath =", exePath);

  if (!exePath) {
    vscode.window.showWarningMessage(
      "YuCode Agent Runtime bulunamadı. Önce agent-runtime build edilmeli."
    );
    return;
  }

  runtimeProcess = cp.spawn(exePath, [], {
    cwd: path.dirname(exePath),
    detached: false,
    windowsHide: true
  });

  runtimeProcess.on("error", (err) => {
  console.error("[YuCode Runtime] spawn error:", err);

  vscode.window.showErrorMessage(
    "YuCode Runtime spawn failed: " + err.message
  );
});

  runtimeProcess.stdout?.on("data", (data) => {
    console.log(`[YuCode Runtime] ${data}`);
  });

  runtimeProcess.stderr?.on("data", (data) => {
    console.error(`[YuCode Runtime] ${data}`);
  });

  runtimeProcess.on("exit", (code) => {
    console.log(`[YuCode Runtime] exited with code ${code}`);
    runtimeProcess = undefined;
  });

  for (let i = 0; i < 30; i++) {
    if (await isRuntimeOnline()) {
        return;
    }

    await sleep(500);
}
}

async function isRuntimeOnline(): Promise<boolean> {
  try {
    const response = await fetch(`${AGENT_URL}/api/status`);
    return response.ok;
  } catch {
    return false;
  }
}

function findRuntimeExecutable(context: vscode.ExtensionContext): string | undefined {
  const config = vscode.workspace.getConfiguration("yucode");
  const configuredRuntime = config.get<string>("runtimePath");

  if (configuredRuntime && fs.existsSync(configuredRuntime)) {
    return configuredRuntime;
  }

  const embeddedRuntime = path.join(
    context.extensionPath,
    "bin",
    "yucode-agent.exe"
  );

  if (fs.existsSync(embeddedRuntime)) {
    return embeddedRuntime;
  }

  return undefined;
}

function sleep(ms: number): Promise<void> {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function getWebviewHtml(): string {
  return `
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
  <style>
  :root {
  --yc-bg: #050505;
  --yc-panel: #0d0d0d;
  --yc-border: #1a1a1a;

  --yc-cyan: #00d9ff;
  --yc-cyan-hover: #00c3e6;

  --yc-text: #d7faff;
  --yc-muted: #7aa7b0;
}

    body {
  background: var(--yc-bg);
  color: var(--yc-text);
}

    textarea {
      width: 100%;
      height: 110px;
      background: var(--vscode-input-background);
      color: var(--vscode-input-foreground);
      border: 1px solid var(--vscode-input-border);
      padding: 8px;
      box-sizing: border-box;
      resize: vertical;
    }

    button {
      margin-top: 8px;
      margin-right: 4px;
      padding: 6px 9px;
      cursor: pointer;
      background: var(--vscode-button-background);
      color: var(--vscode-button-foreground);
      border: none;
      border-radius: 3px;
    }

    .status.online {
  background: rgba(0, 217, 255, 0.10);
  color: var(--yc-cyan);
  border: 1px solid rgba(0, 217, 255, 0.25);
}

.status.offline {
  background: rgba(248, 81, 73, 0.10);
  color: #f85149;
  border: 1px solid rgba(248, 81, 73, 0.25);
}

    #run {
  background: var(--yc-cyan);
  color: #000;
  font-weight: 600;
}

#run:hover {
  background: var(--yc-cyan-hover);
}

    button.secondary {
      background: var(--vscode-button-secondaryBackground);
      color: var(--vscode-button-secondaryForeground);
    }

    button.danger {
      background: #8b2c2c;
      color: white;
    }

    pre {
      white-space: pre-wrap;
      background: var(--vscode-textCodeBlock-background);
      padding: 10px;
      overflow: auto;
      max-height: 220px;
      font-size: 12px;
    }

    .change {
      border: 1px solid var(--vscode-panel-border);
      padding: 10px;
      margin-top: 10px;
      border-radius: 4px;
    }

    .file {
      opacity: 0.75;
      font-size: 11px;
      word-break: break-all;
      margin-top: 4px;
    }

    .explanation {
      margin-top: 8px;
      font-size: 12px;
    }

    .muted {
      opacity: 0.7;
    }

    .step {
  border-left: 2px solid var(--vscode-button-background);
  padding-left: 8px;
  margin: 8px 0;
}

.step-title {
  font-weight: bold;
  font-size: 12px;
}

.step-meta {
  opacity: 0.75;
  font-size: 11px;
  word-break: break-all;
  margin-top: 2px;
}

.quick-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  margin: 8px 0;
}

.quick-actions button {
  margin-top: 0;
}

select {
  width: 100%;
  margin-bottom: 8px;
  background: var(--vscode-dropdown-background);
  color: var(--vscode-dropdown-foreground);
  border: 1px solid var(--vscode-dropdown-border);
  padding: 6px;
}

.error-item {
  border-left: 2px solid #d16969;
  padding-left: 8px;
  margin: 6px 0;
  font-size: 12px;
  word-break: break-all;
}

input {
  width: 100%;
  margin: 4px 0 8px 0;
  background: var(--vscode-input-background);
  color: var(--vscode-input-foreground);
  border: 1px solid var(--vscode-input-border);
  padding: 6px;
  box-sizing: border-box;
}

.status-row {
  padding: 4px 0;
  font-size: 12px;
  word-break: break-all;
}

.diff-preview {
  margin-top: 8px;
  background: var(--vscode-textCodeBlock-background);
  border: 1px solid var(--vscode-panel-border);
  border-radius: 4px;
  overflow: auto;
  max-height: 180px;
  font-family: var(--vscode-editor-font-family);
  font-size: 12px;
}

.diff-line {
  white-space: pre;
  padding: 1px 6px;
}

.diff-add {
  color: #4ec96b;
}

.diff-remove {
  color: #f07178;
}

.diff-context {
  opacity: 0.75;
}

.primary-actions {
  display: flex;
  gap: 6px;
  margin: 10px 0;
}

.primary-actions button {
  flex: 1;
}

.advanced {
  margin: 8px 0 14px 0;
  font-size: 12px;
  opacity: 0.9;
}

.advanced summary {
  cursor: pointer;
  color: var(--vscode-descriptionForeground);
  margin-bottom: 8px;
}

.advanced-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.advanced-actions button {
  margin: 0;
  font-size: 11px;
  padding: 5px 7px;
}

.status {
  display: flex;
  align-items: center;
  gap: 6px;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #3fb950;
}

.status.offline .status-dot {
  background: #f85149;
}
  </style>
</head>
<body>
  <h3>YuCode Agent</h3>

  <div id="status" class="status offline">Checking runtime...</div>

  <textarea id="prompt" placeholder="Ask YuCode to edit, refactor, fix..."></textarea>

  <div class="primary-actions">
  <button id="run">Run</button>
  <button class="secondary" id="refresh">Refresh</button>
</div>

<details class="advanced">
  <summary>Advanced</summary>

  <div class="advanced-actions">
    <button class="secondary" id="checkStatus">Status</button>
    <button class="secondary" id="reindex">Reindex</button>
    <button class="secondary" id="clearSession">Clear Session</button>
    <button class="secondary" id="fixVerify">Fix Verify Error</button>
    <button class="secondary" id="settings">Settings</button>
    <button class="secondary" id="indexStatus">Index Status</button>
    <button class="secondary" id="modelStatus">Model Status</button>
  </div>
</details>

  <h4>Result</h4>
  <pre id="result" class="muted">No result yet.</pre>

  <h4>Pending Changes</h4>
  <div id="changes" class="muted">No pending changes.</div>

  <script>
    const vscode = acquireVsCodeApi();

    const savedState = vscode.getState() || {
  prompt: "",
  mode: "edit",
  result: "No result yet."
};

    const promptEl = document.getElementById("prompt");
    const resultEl = document.getElementById("result");
    const changesEl = document.getElementById("changes");
    const statusEl = document.getElementById("status");
    const modeEl = { value: "auto" };

    promptEl.value = savedState.prompt || "";
if (savedState.lastAgentResult) {
  window.__yucodeLastAgentResult = savedState.lastAgentResult;
  renderAgentResult(savedState.lastAgentResult);
} else {
  resultEl.textContent = savedState.result || "No result yet.";
}

    document.getElementById("run").addEventListener("click", () => {
      const query = promptEl.value.trim();

      if (!query) {
        resultEl.textContent = "Please enter a request.";
        return;
      }

      resultEl.textContent = "Running agent...";

      saveUiState();

      vscode.postMessage({
  type: "runAgent",
  query,
  mode: "auto"
});
    });

    document.getElementById("modelStatus").addEventListener("click", () => {
  resultEl.textContent = "Loading model status...";
  vscode.postMessage({ type: "modelStatus" });
});

    document.getElementById("refresh").addEventListener("click", () => {
      vscode.postMessage({ type: "refreshChanges" });
    });

    document.getElementById("checkStatus").addEventListener("click", () => {
      vscode.postMessage({ type: "checkStatus" });
    });

document.getElementById("fixVerify").addEventListener("click", () => {
  promptEl.value = "Fix the build/test errors from the last verification output.";

  vscode.postMessage({
    type: "runAgent",
    query: promptEl.value,
    mode: "fix",
    useLastVerifyOutput: true
  });
});

document.getElementById("indexStatus").addEventListener("click", () => {
  resultEl.textContent = "Loading index status...";
  vscode.postMessage({ type: "indexStatus" });
});

document.getElementById("reindex").addEventListener("click", () => {
  resultEl.textContent = "Reindexing workspace...";
  vscode.postMessage({ type: "reindex" });
});

document.getElementById("clearSession").addEventListener("click", () => {
  vscode.postMessage({ type: "clearSession" });
});

function saveUiState() {
  vscode.setState({
    prompt: promptEl.value,
    mode: modeEl.value,
    result: resultEl.textContent,
    lastAgentResult: window.__yucodeLastAgentResult || null
  });
}

promptEl.addEventListener("input", saveUiState);

window.addEventListener("message", event => {
      const message = event.data;

if (message.type === "agentStreamStart") {
  window.__yucodeStreamText = "";
  resultEl.textContent = "";
  saveUiState();
}

if (message.type === "agentStreamStatus") {
  resultEl.textContent += "\\n[" + message.message + "]\\n";
  saveUiState();
}

if (message.type === "agentStreamToken") {
  window.__yucodeStreamText =
    (window.__yucodeStreamText || "") + (message.chunk || "");

  resultEl.textContent = window.__yucodeStreamText;
  saveUiState();
}

if (message.type === "agentResult") {
  window.__yucodeStreamText = "";
  window.__yucodeLastAgentResult = message.result;

  renderAgentResult(message.result);
  saveUiState();
}

if (message.type === "settingsData") {
  renderSettings(message.config);
}

if (message.type === "modelStatusResult") {
  renderModelStatus(message.result);
}

if (message.type === "applyResult") {
  renderApplyResult(message.result);
}

if (message.type === "testLlmResult") {
  resultEl.textContent = JSON.stringify(message.result, null, 2);
}

if (message.type === "testEmbeddingResult") {
  resultEl.textContent = JSON.stringify(message.result, null, 2);
}

if (message.type === "indexStatusResult") {
  renderIndexStatus(message.result);
}

if (message.type === "settingsSaved") {
  resultEl.textContent = JSON.stringify(message.result, null, 2);
}

if (message.type === "reindexResult") {
  resultEl.textContent = JSON.stringify(message.result, null, 2);
}

if (message.type === "embeddedRuntimeStatusResult") {
  resultEl.textContent = JSON.stringify(message.result, null, 2);
}

if (message.type === "clearSessionResult") {
  resultEl.textContent = JSON.stringify(message.result, null, 2);
}

      if (message.type === "changes") {
        renderChanges(message.changes);
      }

      if (message.type === "status") {
  statusEl.className =
    "status " + (message.online ? "online" : "offline");

  if (message.online) {
    const backendLabel = String(message.backend || "cpu")
  .replace("cuda13", "CUDA 13")
  .replace("cuda12", "CUDA 12")
  .replace("vulkan", "Vulkan")
  .replace("cpu", "CPU");

statusEl.textContent = "Connected · " + backendLabel;
  } else {
    statusEl.textContent = "Offline";
  }
}

      if (message.type === "error") {
        resultEl.textContent = "Error: " + message.error;
      }
    });

    function cleanAgentMessage(text) {
  if (!text) {
    return "";
  }

  let cleaned = text.trim();
  const fence = String.fromCharCode(96, 96, 96);

  if (cleaned.startsWith(fence + "json")) {
    cleaned = cleaned
      .replace(fence + "json", "")
      .replace(fence, "")
      .trim();
  }

  if (cleaned.startsWith("{")) {
    try {
      const obj = JSON.parse(cleaned);

      return (
        obj.summary ||
        obj.explanation ||
        obj.message ||
        cleaned
      );
    } catch {
      return cleaned;
    }
  }

  return cleaned;
}

    function renderAgentResult(result) {
    window.__yucodeLastAgentResult = result;
  resultEl.innerHTML = "";

  const summary = document.createElement("div");
  let finalMessage = result.message || "";

if (!finalMessage && result.steps) {
  const doneStep = result.steps.find(s => s.action === "done");
  if (doneStep) {
    finalMessage =
      doneStep.output ||
      doneStep.explanation ||
      "";
  }
}

finalMessage = cleanAgentMessage(finalMessage);

summary.textContent = finalMessage || "Agent finished.";
  summary.style.marginBottom = "8px";

  const status = document.createElement("div");
  status.textContent = result.success ? "Success" : "Failed";
  status.style.fontWeight = "bold";
  status.style.marginBottom = "8px";

  resultEl.appendChild(status);
  resultEl.appendChild(summary);

  if (result.pending_change_ids && result.pending_change_ids.length > 0) {
    const pending = document.createElement("div");
    pending.textContent =
      "Pending changes: " + result.pending_change_ids.join(", ");
    pending.style.marginBottom = "8px";
    resultEl.appendChild(pending);
  }

  if (!result.steps || result.steps.length === 0) {
    return;
  }

  const list = document.createElement("div");

  for (const step of result.steps) {
    const item = document.createElement("div");
    item.className = "step";

    const title = document.createElement("div");
    title.className = "step-title";
    title.textContent =
      (step.success ? "✓ " : "× ") +
      (step.action || "unknown");

    const meta = document.createElement("div");
    meta.className = "step-meta";

    const parts = [];

    if (step.query) parts.push("query: " + step.query);
    if (step.file) parts.push("file: " + step.file);
    if (step.command) parts.push("command: " + step.command);
    if (step.explanation) parts.push(step.explanation);

    meta.textContent = parts.join(" | ");

    item.appendChild(title);
    item.appendChild(meta);

    if (step.output && step.action !== "done") {
  const output = document.createElement("pre");
  output.textContent = step.output;
  item.appendChild(output);
}

    list.appendChild(item);
  }

  resultEl.appendChild(list);

  saveUiState();
}

function renderApplyResult(result) {
  resultEl.innerHTML = "";

  const title = document.createElement("div");
  title.style.fontWeight = "bold";
  title.textContent = result.success
    ? "Change applied"
    : "Apply failed";

  if (
  result.success &&
  (!result.errors || result.errors.length === 0)
) {
  const ok = document.createElement("div");
  ok.style.marginTop = "8px";
  ok.style.fontWeight = "bold";
  ok.textContent = "✅ Verification passed";

  resultEl.appendChild(ok);
}

  resultEl.appendChild(title);

  if (
  result.success &&
  (!result.errors || result.errors.length === 0)
) {
  const ok = document.createElement("div");
  ok.style.marginTop = "8px";
  ok.style.fontWeight = "bold";
  ok.textContent = "✅ Verification passed";

  resultEl.appendChild(ok);
}

  if (result.verify_command) {
    const cmd = document.createElement("div");
    cmd.style.marginTop = "8px";
    cmd.textContent = "Verify: " + result.verify_command;
    resultEl.appendChild(cmd);
  }

  if (result.errors && result.errors.length > 0) {
    const heading = document.createElement("div");
    heading.style.marginTop = "10px";
    heading.style.fontWeight = "bold";
    heading.textContent = "Errors";
    resultEl.appendChild(heading);

    for (const err of result.errors) {
      const item = document.createElement("div");
      item.className = "error-item";
      item.textContent =
        err.file_path + ":" + err.line + ":" + err.column + " - " + err.message;

      resultEl.appendChild(item);
    }

    const fixBtn = document.createElement("button");
fixBtn.textContent = "Fix Verify Errors";

fixBtn.onclick = () => {
  vscode.postMessage({
    type: "runAgent",
    query: "Fix the build/test errors from the last verification output.",
    mode: "auto",
    useLastVerifyOutput: true
  });
};

resultEl.appendChild(fixBtn);
  }

  const fixBtn = document.createElement("button");
fixBtn.textContent = "Fix Verify Errors";

fixBtn.onclick = () => {
  vscode.postMessage({
    type: "runAgent",
    query: "Fix the build/test errors from the last verification output.",
    mode: "auto",
    useLastVerifyOutput: true
  });
};

resultEl.appendChild(fixBtn);

  if (result.verify_output) {
    const output = document.createElement("pre");
    output.textContent = result.verify_output;
    resultEl.appendChild(output);
  }
}

function renderIndexStatus(status) {
  resultEl.innerHTML = "";

  const title = document.createElement("div");
  title.style.fontWeight = "bold";
  title.textContent = "Index Status";
  resultEl.appendChild(title);

  const rows = [
    ["Workspace", status.workspace_path || "-"],
    ["Has Index", String(status.has_index)],
    ["Files", String(status.files)],
    ["Symbols", String(status.symbols)],
    ["References", String(status.references)],
    ["Calls", String(status.calls)],
    ["Embedding Enabled", String(status.embedding_enabled)],
    ["Embedding Chunks", String(status.embedding_chunks)],
    ["Semantic Ready", String(status.semantic_ready)]
  ];

  for (const row of rows) {
    const item = document.createElement("div");
    item.className = "status-row";
    item.textContent = row[0] + ": " + row[1];
    resultEl.appendChild(item);
  }
}

function renderModelStatus(status) {
  resultEl.innerHTML = "";

  const title = document.createElement("div");
  title.style.fontWeight = "bold";
  title.textContent = "YuCode Local Model Status";
  resultEl.appendChild(title);

  const rows = [
    ["Embedded Runtime Enabled", String(status.embedded_runtime_enabled)],
    ["Runtime Running", String(status.runtime_running)],
    ["Server Exists", String(status.server_exists)],
    ["Server Path", status.server_path || "-"],
    ["Model Exists", String(status.model_exists)],
    ["Model Path", status.model_path || "-"],
    ["Base URL", status.base_url || "-"],
    ["Port", String(status.port)]
  ];

  for (const row of rows) {
    const item = document.createElement("div");
    item.className = "status-row";
    item.textContent = row[0] + ": " + row[1];
    resultEl.appendChild(item);
  }

  if (!status.server_exists || !status.model_exists) {
    const warning = document.createElement("div");
    warning.style.marginTop = "10px";
    warning.style.fontWeight = "bold";
    warning.textContent =
      "YuCode Local is not fully ready. Check llama-server.exe and model path.";
    resultEl.appendChild(warning);
  }
}

function diffStats(oldText, newText) {
  const oldLines = (oldText || "").split("\\n");
  const newLines = (newText || "").split("\\n");

  const oldSet = new Set(oldLines);
  const newSet = new Set(newLines);

  let added = 0;
  let removed = 0;

  for (const line of newLines) {
    if (!oldSet.has(line)) added++;
  }

  for (const line of oldLines) {
    if (!newSet.has(line)) removed++;
  }

  return { added, removed };
}

function renderDiffPreview(unifiedDiff) {
  const diffBox = document.createElement("div");
  diffBox.className = "diff-preview";

  for (const line of (unifiedDiff || "").split("\\n")) {
    const row = document.createElement("div");
    row.className = "diff-line";

    if (line.startsWith("+") && !line.startsWith("+++")) {
      row.classList.add("diff-add");
    } else if (line.startsWith("-") && !line.startsWith("---")) {
      row.classList.add("diff-remove");
    } else {
      row.classList.add("diff-context");
    }

    row.textContent = line;
    diffBox.appendChild(row);
  }

  return diffBox;
}

    function renderChanges(changes) {
      if (!changes || changes.length === 0) {
        changesEl.className = "muted";
        changesEl.textContent = "No pending changes.";
        return;
      }

      changesEl.className = "";
      changesEl.innerHTML = "";

      for (const change of changes) {
        const root = document.createElement("div");
        root.className = "change";

        const title = document.createElement("strong");
        title.textContent = change.id;

        const file = document.createElement("div");
        file.className = "file";
        const fileCount =
  change.files && change.files.length > 0
    ? change.files.length
    : 1;

file.textContent =
  fileCount > 1
    ? fileCount + " files"
    : change.file_path;

        const explanation = document.createElement("div");
        explanation.className = "explanation";
        explanation.textContent = change.explanation || "No explanation.";

        const openBtn = document.createElement("button");
        openBtn.className = "secondary";
        openBtn.textContent = "Open";
        openBtn.onclick = () => {
          vscode.postMessage({
            type: "openFile",
            filePath: change.file_path
          });
        };

        const diffBtn = document.createElement("button");
        diffBtn.className = "secondary";
        diffBtn.textContent = "Diff";
        diffBtn.onclick = () => {
          vscode.postMessage({
            type: "openDiff",
            id: change.id,
            filePath: change.file_path,
            newContent: change.new_content
          });
        };

        const applyBtn = document.createElement("button");
applyBtn.textContent = "Apply & Verify";
        applyBtn.onclick = () => {
  resultEl.textContent = "Applying change and running verification...";

  vscode.postMessage({
    type: "applyChange",
    id: change.id
  });
};

        const rejectBtn = document.createElement("button");
        rejectBtn.className = "danger";
        rejectBtn.textContent = "Reject";
        rejectBtn.onclick = () => {
  resultEl.textContent = "Rejecting change...";

  vscode.postMessage({
    type: "rejectChange",
    id: change.id
  });
};

        root.appendChild(title);
        root.appendChild(file);
        root.appendChild(explanation);
        const stats = diffStats(
  change.old_content || "",
  change.new_content || ""
);

const statEl = document.createElement("div");
statEl.className = "explanation";
statEl.textContent =
  "+" + stats.added + " / -" + stats.removed + " line changes";

root.appendChild(statEl);
if (change.files && change.files.length > 0) {
  for (const fileChange of change.files) {
    const fileTitle = document.createElement("div");
    fileTitle.className = "file";
    fileTitle.textContent = fileChange.file_path;
    root.appendChild(fileTitle);

    if (fileChange.unified_diff) {
      root.appendChild(renderDiffPreview(fileChange.unified_diff));
    }
  }
} else if (change.unified_diff) {
  root.appendChild(renderDiffPreview(change.unified_diff));
}
        root.appendChild(openBtn);
        root.appendChild(diffBtn);
        root.appendChild(applyBtn);
        root.appendChild(rejectBtn);

        changesEl.appendChild(root);
      }
    }

document.getElementById("settings").addEventListener("click", () => {
  vscode.postMessage({ type: "openSettings" });
});

function renderSettings(config) {
  resultEl.innerHTML = "";

  const fields = [
  "llm_provider",
  "llm_base_url",
  "llm_model",
  "llm_api_key",
  "embedding_provider",
  "embedding_base_url",
  "embedding_model",
  "embedded_server_path",
  "embedded_model_path",
  "embedded_runtime_port",
  "embedded_context_size",
  "embedded_gpu_layers"
];

  const title = document.createElement("div");
  title.style.fontWeight = "bold";
  title.textContent = "YuCode Settings";
  resultEl.appendChild(title);

  const presetLabel = document.createElement("label");
presetLabel.textContent = "Provider Preset";

const preset = document.createElement("select");
preset.id = "setting_provider_preset";

const presets = [
  { id: "lmstudio", label: "LM Studio" },
  { id: "ollama", label: "Ollama" },
  { id: "groq", label: "Groq" },
  { id: "custom", label: "Custom" },
  { id: "yucode-local", label: "YuCode Local" }
];

for (const item of presets) {
  const option = document.createElement("option");
  option.value = item.id;
  option.textContent = item.label;

  if (config.llm_provider === item.id) {
    option.selected = true;
  }

  preset.appendChild(option);
}

resultEl.appendChild(presetLabel);
resultEl.appendChild(preset);

  const enabledLabel = document.createElement("label");
  enabledLabel.textContent = "Embedding Enabled";

  const enabled = document.createElement("input");
  enabled.type = "checkbox";
  enabled.checked = !!config.embedding_enabled;
  enabled.id = "setting_embedding_enabled";

  resultEl.appendChild(enabledLabel);
  resultEl.appendChild(enabled);

  const embeddedLabel = document.createElement("label");
embeddedLabel.textContent = "Embedded Runtime Enabled";

const embeddedEnabled = document.createElement("input");
embeddedEnabled.type = "checkbox";
embeddedEnabled.checked = !!config.embedded_runtime_enabled;
embeddedEnabled.id = "setting_embedded_runtime_enabled";

resultEl.appendChild(embeddedLabel);
resultEl.appendChild(embeddedEnabled);

  for (const key of fields) {
    const label = document.createElement("label");
    label.textContent = key;

    const input = document.createElement("input");
    input.id = "setting_" + key;
    input.value = config[key] || "";

    resultEl.appendChild(label);
    resultEl.appendChild(input);
  }

  preset.onchange = () => {
  const value = preset.value;

  if (value === "lmstudio") {
    document.getElementById("setting_llm_provider").value = "lmstudio";
    document.getElementById("setting_llm_base_url").value = "http://127.0.0.1:1234/v1";
    document.getElementById("setting_llm_model").value = "local-model";
    document.getElementById("setting_embedding_provider").value = "lmstudio";
    document.getElementById("setting_embedding_base_url").value = "http://127.0.0.1:1234/v1";
    document.getElementById("setting_embedding_model").value = "text-embedding-nomic-embed-text-v1.5";
  }

  if (value === "ollama") {
    document.getElementById("setting_llm_provider").value = "ollama";
    document.getElementById("setting_llm_base_url").value = "http://127.0.0.1:11434/v1";
    document.getElementById("setting_llm_model").value = "qwen3:4b";
    document.getElementById("setting_embedding_provider").value = "ollama";
    document.getElementById("setting_embedding_base_url").value = "http://127.0.0.1:11434";
    document.getElementById("setting_embedding_model").value = "nomic-embed-text";
  }

  if (value === "groq") {
    document.getElementById("setting_llm_provider").value = "groq";
    document.getElementById("setting_llm_base_url").value = "https://api.groq.com/openai/v1";
    document.getElementById("setting_llm_model").value = "qwen/qwen3-32b";
    document.getElementById("setting_embedding_provider").value = "lmstudio";
    document.getElementById("setting_embedding_base_url").value = "http://127.0.0.1:1234/v1";
    document.getElementById("setting_embedding_model").value = "text-embedding-nomic-embed-text-v1.5";
  }

  if (value === "yucode-local") {
  document.getElementById("setting_llm_provider").value = "yucode-local";
  document.getElementById("setting_llm_base_url").value = "http://127.0.0.1:11435/v1";
  document.getElementById("setting_llm_model").value = "code-model";
  document.getElementById("setting_embedded_server_path").value = "runtime/llama-server.exe";
  document.getElementById("setting_embedded_model_path").value = "models/code-model.gguf";
  document.getElementById("setting_embedded_runtime_port").value = "11435";
  document.getElementById("setting_embedded_context_size").value = "32768";
  document.getElementById("setting_embedded_gpu_layers").value = "0";
}
};

  const save = document.createElement("button");
  save.textContent = "Save Settings";
  save.onclick = () => {
    const next = {
  embedding_enabled: document.getElementById("setting_embedding_enabled").checked,
  embedded_runtime_enabled: document.getElementById("setting_embedded_runtime_enabled").checked
};

    for (const key of fields) {
      next[key] = document.getElementById("setting_" + key).value;
    }

    vscode.postMessage({
      type: "saveSettings",
      config: next
    });
  };

  resultEl.appendChild(save);

  const test = document.createElement("button");
test.textContent = "Test LLM";
test.onclick = () => {
  resultEl.textContent = "Testing LLM provider...";
  vscode.postMessage({ type: "testLlm" });
};

const testEmbedding = document.createElement("button");
testEmbedding.textContent = "Test Embedding";
testEmbedding.onclick = () => {
  resultEl.textContent = "Testing embedding provider...";
  vscode.postMessage({ type: "testEmbedding" });
};

resultEl.appendChild(testEmbedding);

const testEmbedded = document.createElement("button");
testEmbedded.textContent = "Start/Test YuCode Local Runtime";
testEmbedded.onclick = () => {
  resultEl.textContent = "Checking embedded runtime...";
  vscode.postMessage({ type: "embeddedRuntimeStatus" });
};

resultEl.appendChild(testEmbedded);

resultEl.appendChild(test);
}
  </script>
</body>
</html>
`;
}