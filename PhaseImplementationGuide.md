## The Master Setup: Project Ultimatum

**Target Directory:** `/home/someone/Ultimatum` **Language:** Python 3.12 (Strict requirement to preserve Open3D/RealSense compatibility). **Frontend:** React/Vue (for WebUI), `prompt_toolkit` (for TUI). **Backend:** FastAPI (WebSockets + REST), `llama-cpp-python` (with CUDA/CuBLAS enabled).

### Directory Skeleton Blueprint

Before writing a single line of logic, the physical directory must be scaffolded to enforce the sandbox.

- `/Ultimatum/core/` - Immutable Python logic (Memory, Routing, SWD).
    
- `/Ultimatum/plugins/` - Toggleable modules (Physics, Vision, Cyber).
    
- `/Ultimatum/models/` - The storage vault for downloaded `.gguf` files.
    
- `/Ultimatum/memory/` - The SQLite FTS5 database and markdown logs.
    
- `/Ultimatum/workspace/` - The absolute boundary for AI file editing.
    
- `/Ultimatum/sandbox/` - The `/tmp` equivalent for isolated code testing.
    

## PHASE 1: The Sovereign Foundation & Absolute Security

_This phase builds the unkillable root of the program. It establishes the hardware connections, downloads the models, and constructs the impenetrable security cage._

### 1. Strict Directory Sandboxing (Path Confinement)

- **What it is:** A hardcoded security perimeter. The AI is physically blind to anything outside `/home/someone/Ultimatum`.
    
- **How to use it:** Operates invisibly. If an agent tries to `cd ../../etc/`, the system throws a `Permission Denied` error to the model. Users can whitelist external drives via the WebUI settings.
    
- **How to implement:** Build a Python utility class `PathValidator`. Every tool or agent that interacts with the `os` or `shutil` libraries must pass its target path through this validator. Use `os.path.abspath` and `os.path.commonpath` to verify the resolved target path strictly starts with `/home/someone/Ultimatum`. If it does not, raise a custom `SandboxViolationError`.
    

### 2. Built-in Model Downloader & Manager

- **What it is:** An integrated hub pulling directly from HuggingFace to download, manage, and assign `.gguf` models.
    
- **How to use it:** Open the "Model Hub" tab in the WebUI. Click to download pre-curated specialist models (e.g., Qwen-Coder, Llama-3-8B-Instruct). Assign them to "Dormant" (storage) or map them to active roles (e.g., "Main Orchestrator").
    
- **How to implement:** Utilize the `huggingface_hub` Python library. Create a `models.json` bootstrap file containing a curated list of highly efficient, quantized GGUF links (math specialists, coding specialists). Build a FastAPI route that triggers `hf_hub_download` asynchronously, displaying a progress bar in the WebUI via WebSockets. Update an internal SQLite table mapping the downloaded file paths to active or dormant states.
    

### 3. Direct `llama.cpp` Integration

- **What it is:** Bare-metal execution of language models without Docker or API overhead.
    
- **How to use it:** Invisible backend process.
    
- **How to implement:** Install `llama-cpp-python` compiled with `CMAKE_ARGS="-DGGML_CUDA=on"`. Build a singleton `ModelManager` class that holds the loaded `Llama` object in system memory, allowing for direct manipulation of the `n_gpu_layers` argument based on the user's scale settings.
    

### 4. Core Strict Write Discipline (SWD) & SHA-256 Snapshots

- **What it is:** Cryptographic file hashing that prevents irreversible code destruction.
    
- **How to use it:** Automatic. If an agent wants to rewrite a Python file, the system hashes the file, lets the agent edit it, and logs the change. If the code breaks, type `/rollback`.
    
- **How to implement:** Before any `write` operation, the `SWDManager` reads the target file, computes `hashlib.sha256().hexdigest()`, and saves a copy of the file to a hidden `.receipts/` directory. Create a JSON log mapping the hash, timestamp, and model used.
    

### 5. Isolated Runs & Atomic Writes

- **What it is:** Code generation happens in a quarantine zone before affecting your real project.
    
- **How to use it:** Invisible backend safety net.
    
- **How to implement:** When an agent writes a file, route the path to `/Ultimatum/sandbox/`. Run syntax linters (e.g., `ast.parse` for Python) on the sandboxed file. If it passes, use `os.replace()` to move it to the real `/Ultimatum/workspace/` directory. `os.replace` is an atomic operation in POSIX, guaranteeing the file is never half-written.
    

### 6. Security Air-Gap & The Zero-Trust Gatekeeper

- **What it is:** The visual and systemic barrier preventing models from running dangerous commands without your permission.
    
- **How to use it:** When a model requests a heavy tool or a system command, a "Pending Approvals" sidebar pops out in the WebUI. Click "Allow", "Deny", or "Modify".
    
- **How to implement:** Intercept the LLM's structured JSON tool-call output. Instead of executing it, push the JSON to an `asyncio.Queue`. Send a WebSocket event to the React frontend to render the approval card in the sidebar. Suspend the active Python thread using `await` until the React frontend sends back an `APPROVED` or `DENIED` WebSocket message.
    

### 7. Unified Local Interfaces (TUI & WebUI Base)

- **What it is:** The dual-cockpit of Project Ultimatum.
    
- **How to use it:** Run `python main.py --tui` for the terminal interface, or type `/webui` to pop open the React dashboard in your browser.
    
- **How to implement:** * **Backend:** FastAPI serving REST endpoints and WebSockets.
    
    - **WebUI:** React with TailwindCSS.
        
    - **TUI:** Use Python's `prompt_toolkit` to create split-pane terminal layouts (input bar at the bottom, chat log in the middle, active VRAM/Status bar at the top).
        

## PHASE 2: The Cognitive Engine & Orchestration

_This phase builds the "brain" of the operation. It manages how models talk to each other, how they remember you, and how you visually program their workflows._

### 8. Node-Based Framework Editor (The Obsidian Canvas)

- **What it is:** A visual drag-and-drop programming interface for AI swarms.
    
- **How to use it:** Open the "Framework Canvas" tab in the WebUI. Drag a "User Input" node, connect it to a "Llama-3 (Router)" node. Branch that into "Qwen-Coder" and "Vision Model". Add a "Python Tool" node to the coder. Save this as a custom load order.
    
- **How to implement:** Use `React Flow` on the frontend to render the nodes and edges. When the user clicks "Save", serialize the graph into a JSON Directed Acyclic Graph (DAG) (e.g., `{"nodes": [...], "edges": [...]}`). On the backend, write a `GraphExecutor` class that traverses the DAG using topological sorting, instantiating the assigned models and passing the context string from one node to the next.
    

### 9. Predictive VRAM Swapper

- **What it is:** Aggressive memory management for 8GB GPUs.
    
- **How to use it:** Invisible. Allows you to run a 14B coder and a 3B router on the same machine.
    
- **How to implement:** Create a `VRAMArbitrator` class. When the Node Editor's DAG executes, the arbitrator unloads the current `Llama` object (calling `del model` and `libc.malloc_trim(0)` to force garbage collection), reallocates the new `.gguf` from the SSD, and adjusts `n_gpu_layers` dynamically based on `psutil` RAM readings.
    

### 10. Semantic Router

- **What it is:** A lightning-fast intent classifier.
    
- **How to use it:** Type a prompt like "Calculate the physics of this image." The router instantly forwards it to the Vision model and the Math model.
    
- **How to implement:** Load a tiny embedding model (like `all-MiniLM-L6-v2` via `sentence-transformers`) permanently into system RAM (takes ~80MB). Compare the cosine similarity of the user's prompt against predefined tool/agent descriptions to pick the highest match.
    

### 11. Synapse 3-Layer Memory & FTS5 Index

- **What it is:** The memory architecture (Buffer, Lattice, Core) backed by a lightning-fast database.
    
- **How to use it:** Natural conversation. The AI remembers things from weeks ago instantly.
    
- **How to implement:** Write chat logs to markdown files in `/Ultimatum/memory/`. At system boot, parse these markdown files and insert them into an SQLite database created with the `FTS5` (Full-Text Search) virtual table extension. Use `MATCH` queries to retrieve historical context based on the current prompt's keywords.
    

### 12. `SOUL.md` & `USER.md`

- **What it is:** Immutable identity profiles for both the AI and you.
    
- **How to use it:** Edit them in the WebUI settings. Tell the AI to be sarcastic, or record that you only code in Python.
    
- **How to implement:** Read these markdown files into Python strings at startup. Always inject them as the absolute first lines of the `system` message array before sending it to `llama.cpp`.
    

### 13. The Dream Cycle & Contextual Amnesia

- **What it is:** Background memory consolidation.
    
- **How to use it:** Runs when you aren't using the program.
    
- **How to implement:** Use the `APScheduler` library to run a cron job at 3:00 AM (or after 30 mins of idle time). Load a highly quantized 1.5B model. Feed it the raw chat logs of the day, prompt it to extract key facts into bullet points, append the bullets to `USER.md`, and delete the bloated chat logs.
    

### 14. 4-Tier State Machine

- **What it is:** Power management (Deep Sleep, Ambient, Active, Overdrive).
    
- **How to use it:** The system scales hardware usage based on this state.
    
- **How to implement:** A global Python `Enum`. In `Deep Sleep`, all models are purged from RAM. In `Ambient`, only the Wake-Word listener and Semantic Router remain in RAM. In `Active`, a primary model is held in VRAM.

## PHASE 3: The Active Council Loop (Execution & Coding)

_This phase handles the multi-threaded orchestration of agents. It is where logic is parsed, code is tested, and adversarial models debate solutions._

### 14. Subagent Spawning & Multi-Agent Pipeline

- **What it is:** The ability for the primary orchestrator to fork isolated, specialized worker processes (e.g., spawning a "UI Designer" and a "Backend Dev" simultaneously).
    
- **How to use it:** In the WebUI's Framework Canvas, link a master node to multiple worker nodes. When a prompt is submitted, the orchestrator divides the task and delegates it down the tree.
    
- **How to implement:** In `/Ultimatum/core/council/`, build an `AgentSpawner` class using Python's native `multiprocessing` or `asyncio.subprocess`. When a subagent is spawned, the VRAM Swapper serializes the primary orchestrator to system RAM. The subagent is allocated a highly quantized, task-specific GGUF (e.g., 3B parameter coding model), completes its task in `/Ultimatum/sandbox/`, and returns a JSON structured output via `stdout` pipes to the master process.
    

### 15. Shadow Council & Peerless Peer-Review

- **What it is:** A consensus and debate mechanism. Instead of trusting one model's output, the system runs the prompt through multiple adversarial personas.
    
- **How to use it:** Prefix a prompt with `/debate` or toggle the "Shadow Council" switch in the TUI.
    
- **How to implement:** Use `asyncio.gather` to queue multiple inference requests with distinct system prompts (e.g., "You are a pessimistic security auditor", "You are an optimistic feature developer"). Since 8GB VRAM cannot hold 3 large models simultaneously, the VRAM Swapper batches the requests: it loads the model, generates Persona A's response, saves to cache, generates Persona B, and so on. A final "Judge" prompt evaluates the outputs and synthesizes the optimal solution.
    

### 16. Auto-Healing TDD (Test-Driven Development)

- **What it is:** A closed-loop autonomous debugging cycle.
    
- **How to use it:** Submit a prompt with a test command, e.g., `/code "write a sorting algo" --test "pytest sort.py"`.
    
- **How to implement:** In `/Ultimatum/core/execution/tdd_loop.py`, use the `subprocess` module to execute the generated code inside `/Ultimatum/sandbox/`. Capture `stderr` and `stdout`. If the exit code is non-zero, parse the error traceback, append it to the model's message history as a `user` role message (`[SYSTEM: Test failed with output: {stderr}. Fix the code.]`), and re-trigger inference. Cap this at a hard limit of `max_retries=2` to prevent infinite loops.
    

### 17. AST Patching

- **What it is:** Programmatic, surgical code modification. Instead of asking the LLM to rewrite a 1,000-line file, the system patches only the specific function that needs changing.
    
- **How to use it:** Invisible to the user. Triggered when editing large codebases.
    
- **How to implement:** Utilize Python's native `ast` (Abstract Syntax Tree) module. When the LLM outputs a `[FILE_ACTION]` block targeting a specific function, parse the target Python file into an AST, locate the specific `FunctionDef` node, and replace it with the newly generated AST node. Write the modified tree back to the sandbox file using `ast.unparse()`.
    

### 18. Predictive Merge & Git Conflict Prediction

- **What it is:** Anticipates version control conflicts before they occur.
    
- **How to use it:** Triggered automatically during the SWD atomic write phase if working in a Git repository.
    
- **How to implement:** Use the `GitPython` library. Before executing `os.replace()` to move a sandboxed file to the workspace, check the Git status of the target branch. Compare the unified diff of the proposed AI change against any uncommitted human changes in the working tree. If overlap is detected, halt the atomic write and push a `Drift-Gating` alert to the Pending Approvals sidebar.
    

## PHASE 4: The Multimodal Sensorium

_This phase attaches the sensory organs to the core engine. It allows the AI to see, hear, speak, and monitor your physical and digital environment._

### 19. Real-Time Vision & OCR

- **What it is:** The visual processing pipeline for analyzing UI layouts, diagrams, and physical text.
    
- **How to use it:** Drag and drop an image into the WebUI chat, or pipe an image path in the CLI (`/vision /home/someone/Pictures/error.png`).
    
- **How to implement:** In `/Ultimatum/plugins/sensorium/vision.py`, integrate `pytesseract` for raw text extraction and the `transformers` library to load a tiny vision-language model like `vikhyatk/moondream2`. Load the vision model into VRAM only when an image is detected in the prompt queue, instantly unloading it once the image is converted to a dense markdown description.
    

### 20. High-Speed Local Audio Suite (Whisper STT)

- **What it is:** Secure, offline speech-to-text transcription.
    
- **How to use it:** Press and hold the designated Push-to-Talk hotkey globally, or click the microphone icon in the WebUI.
    
- **How to implement:** Use the `faster-whisper` library (CTranslate2 backend). Keep a small, quantized Whisper model (e.g., `tiny.en` or `base.en`) permanently loaded in system RAM (requires < 500MB). Use `sounddevice` to capture raw PCM audio from the microphone array.
    

### 21. Local Speech Synthesis (Piper TTS)

- **What it is:** Instantaneous offline voice generation.
    
- **How to use it:** Toggle `/voice on`. The AI reads its responses aloud.
    
- **How to implement:** In `/Ultimatum/plugins/sensorium/voice.py`, implement `piper-tts`. Piper uses the ONNX runtime, which is incredibly CPU-efficient and requires zero VRAM. Pipe the text stream generated by `llama.cpp` directly into Piper to synthesize audio chunks on the fly, playing them back via `sounddevice` or `pyaudio` before the text generation even finishes.
    

### 22. Voice Synthesis & Soul Cloning Matrix

- **What it is:** The creation of custom, programmatic voice profiles.
    
- **How to use it:** Type `/voice generate "A calm, analytical female voice with a British accent"`.
    
- **How to implement:** Utilize a framework like `Coqui TTS` (XTTSv2) if VRAM permits, or manipulate Piper's speaker embeddings. Write a script that takes the LLM's interpretation of your text description, maps it to a latent acoustic vector, and saves the `.onnx` model or embedding profile to `/Ultimatum/memory/voices/`.
    

### 23. Screen Monitoring & OS Signal Interceptor

- **What it is:** Ambient awareness of your digital workspace.
    
- **How to use it:** Toggle "Screen Context" in the WebUI.
    
- **How to implement:** Use the `mss` library for high-speed, cross-platform screenshot capturing without blocking the main thread. Use `psutil` and X11/Wayland bindings (e.g., `ewmh` or `dbus` on CachyOS) to read the active window title. Dump this string (e.g., `[ACTIVE WINDOW: Visual Studio Code - main.py]`) invisibly into the system prompt at the start of every inference cycle.
    

### 24. Typing Cadence Analysis & Vocal Stress Micro-Tremor

- **What it is:** Biometric-free stress detection.
    
- **How to use it:** Runs passively. If you start typing erratically or speaking at a higher pitch, the UI simplifies.
    
- **How to implement:** In `/Ultimatum/core/psychometrics/cadence.py`, implement a `pynput` listener. Calculate the Inter-Keystroke Interval (IKI) and backspace frequency. For voice, run a fast Fast Fourier Transform (FFT) over the audio buffer using `scipy.fft` to detect pitch variance. If the stress heuristic exceeds a threshold, emit a WebSocket event to the frontend to trigger the `Temporal Collapse` UI state.
    

## PHASE 5 EXPANSION: Volume 1 (Cognition & Psychology)

_These modules reside in `/home/someone/Ultimatum/plugins/cognitive/` and `/plugins/pedagogy/`. They are toggleable sub-systems designed to monitor your behavior, optimize your learning, and passively generate insights._

### 1. Emotional Spectrum Tagging & Eidetic Indexing

- **What it is:** A memory enrichment system that tags your text logs with detected emotional states and visual screenshots, allowing the AI to recall _how_ you felt during a specific coding session.
    
- **How to use it:** Handled passively. If you search `/memory "that time I was frustrated with React"`, the FTS5 index uses the emotional tags to find the exact session and displays the blurred screenshot of your IDE.
    
- **How to implement:** In `/Ultimatum/plugins/cognitive/eidetic.py`, hook into the OS using `mss` to take a low-resolution screenshot (blurred via `Pillow` to save space/privacy) whenever an error traceback is detected in the terminal. Simultaneously, pass your prompts through an offline sentiment classifier (e.g., `distilbert-base-uncased-emotion` loaded via `transformers`). Save the sentiment label (e.g., `anger`, `joy`, `focus`) as metadata in the SQLite FTS5 database alongside the `MEMORY.md` file path and the image hash.
    

### 2. Continuous Cognition & Need Analysis Engine

- **What it is:** A background execution loop that allows the AI to "think" while you are away, predicting what you need before you ask.
    
- **How to use it:** Toggle "Background Cognition" in the WebUI. When you return, the AI might greet you with: _"I noticed we struggled with memory leaks yesterday. I compiled a summary of Python garbage collection best practices for you."_
    
- **How to implement:** In `/Ultimatum/core/background/cognition.py`, use the `asyncio` event loop. When the `4-Tier State Machine` enters `Ambient` mode (user is away), load a highly quantized 1.5B model. Feed it the last 48 hours of `MEMORY.md` logs and prompt it to identify unresolved questions or recurring struggles. Save its output to a `insights.json` queue, which the main orchestrator reads and presents to you upon your return.
    

### 3. Shadow Work Initiation & Unconscious Bias Amplifier

- **What it is:** Psychological mirrors. The system detects topics you actively avoid discussing or coding tasks you constantly procrastinate on, gently forcing you to confront them.
    
- **How to use it:** Activated via `/toggle shadow_work`. The agent acts as a Socratic Gadfly during casual chat.
    
- **How to implement:** In `/Ultimatum/plugins/psychology/shadow.py`, write an analysis script that compares your "To-Do" list or Git issues against your actual coding logs. If a task has been delayed >5 times, or if sentiment analysis detects you changing the subject when a specific language (e.g., C++) is mentioned, the plugin injects a system prompt override: `[SYSTEM: The user is avoiding X. Subtly steer the conversation to explore why they are avoiding it without being confrontational.]`
    

### 4. Dream Journal Correlator & Restorative Nostalgia

- **What it is:** A wellness feature that tracks your offline mental state and uses past positive coding memories to boost your mood during highly frustrating debugging sessions.
    
- **How to use it:** Type `/journal "I dreamt about failing a test"` or let the system trigger nostalgia automatically when frustration spikes.
    
- **How to implement:** Create a dedicated `/Ultimatum/memory/journal.md`. Use the `sentence-transformers` library to compute vector embeddings of your journal entries against your coding logs to find Jungian overlaps. If the `Typing Cadence Analyzer` detects high frustration (erratic backspaces), trigger the `Restorative Nostalgia` function: query the FTS5 database for a memory tagged with `joy` or `triumph` and inject it into the LLM's context: `[SYSTEM: The user is stressed. Remind them of the time they successfully deployed the VRAM Swapper last month to boost morale.]`
    

### 5. Spaced Repetition (SM-2) & Blank Spot Auditing

- **What it is:** An integrated learning management system that forces you to retain new syntax or concepts rather than relying purely on the AI to write them for you.
    
- **How to use it:** Type `/review`. The AI will quiz you on recent concepts.
    
- **How to implement:** In `/Ultimatum/plugins/pedagogy/sm2.py`, maintain a database table for `Flashcards`. When you ask the AI to explain a new concept (e.g., "How does asyncio.gather work?"), the AI automatically generates a Q&A pair and stores it. The SM-2 algorithm updates the `next_review_date` based on your self-reported score (0-5) using the formula: I(n)=I(n−1)×EF. When `/review` is called, query SQLite for all cards where `next_review_date <= TODAY`.
    

### 6. Zettelkasten 2.0 & Hypothesis Generation

- **What it is:** A deep-utility research framework that links your notes and automatically proposes novel, testable ideas based on the connections.
    
- **How to use it:** Use `/zettel "New note content"`. The AI automatically tags it and links it to existing notes.
    
- **How to implement:** In `/Ultimatum/plugins/utility/zettel.py`, parse notes into markdown files under `/Ultimatum/workspace/notes/`. Upon creation, run the note through an embedding model and store it in `ChromaDB`. Retrieve the top 3 most similar existing notes. Pass the new note and the 3 similar notes to the LLM, prompting it to generate a `Hypothesis` based on the intersection of the data. Append the links and the hypothesis to the bottom of the markdown file.
    

### 7. Auto-Paper Drafting & Verification Chain

- **What it is:** An academic and documentation utility that turns your fragmented notes and research into beautifully formatted, citation-backed LaTeX documents.
    
- **How to use it:** Type `/draft paper "Title"`.
    
- **How to implement:** In `/Ultimatum/plugins/pedagogy/drafter.py`, compile all notes tagged with a specific project ID. Construct a massively structured prompt requiring the LLM to output pure LaTeX syntax. Route the output to `/Ultimatum/sandbox/paper.tex`. Use the `subprocess` module to call `pdflatex` (which must be installed on the host OS). If it compiles successfully, move the PDF to `/Ultimatum/workspace/exports/`. If it fails, feed the LaTeX error log back into the `Auto-Healing TDD` loop for correction.
    

### 8. Learning Style Profiler & Lexical Complexity Trainer

- **What it is:** A dynamic text-formatting engine that adjusts the AI's prose to match your current cognitive load and preferred learning method (e.g., visual analogies vs. raw code).
    
- **How to use it:** Runs invisibly. Adjusted manually via the WebUI settings.
    
- **How to implement:** In `/Ultimatum/core/orchestration/profiler.py`, track interaction metrics (how often you ask "explain simpler" vs "just give me the code"). Store a floating-point value for `Lexical_Density` (0.0 to 1.0). Inject this as a dynamic system instruction: `[SYSTEM: Current Lexical Density is 0.2. Use simple metaphors. Avoid academic jargon. Use short sentences.]`
  
  ## PHASE 5 EXPANSION: Volume 2 (OSINT, Cyber-Physical, & Memetic Immunity)

_These modules reside in `/home/someone/Ultimatum/plugins/osint/`, `/plugins/bio/`, and `/plugins/immunity/`. They are strictly sandboxed and require high-level clearance to interact with external networks or biometric hardware._

### Module A: The Deep Research & OSINT Suite

_This suite transforms the engine into a sovereign intelligence agency. It scrapes, verifies, and permanently archives external data without relying on tracked corporate search APIs._

#### 1. Iterative Deep Research Loop & Web Gatekeeper

- **What it is:** A recursive, multi-threaded web scraper and synthesizer. It doesn't just search; it reads, follows citations, builds an internal vector database, and drafts comprehensive reports backed by primary sources.
    
- **How to use it:** Type `/research "Analyze the supply chain vulnerabilities of TSMC's 2nm process."` The system spawns 3 to 5 subagents.
    
- **How to implement:** In `/Ultimatum/plugins/osint/research_loop.py`, utilize `httpx` and `beautifulsoup4` within an `asyncio` loop for concurrent fetching. Route all traffic through a local rotating proxy or Tor proxy. Downloaded HTML is stripped of JavaScript/CSS, chunked, and embedded using an efficiently quantized sentence-transformer. Embeddings are stored locally in `/Ultimatum/memory/vector_store/`. If a subagent encounters a paywall or CAPTCHA, it gracefully degrades, logging a "Source Blocked" warning, and pivots to an alternative citation.
    

#### 2. SearXNG Meta-Search Hub

- **What it is:** A self-hosted search aggregator that queries Google, Bing, DuckDuckGo, and academic databases simultaneously, completely stripping your IP and search fingerprint.
    
- **How to use it:** Operates invisibly as the backbone for the Deep Research Loop.
    
- **How to implement:** In `/Ultimatum/plugins/osint/searxng.py`, use the `docker` Python SDK to manage a local `searxng/searxng` container bound to `127.0.0.1:8080`. Modify the internal `settings.yml` to disable search history and enforce strict JSON output. The Python plugin sends requests to `http://127.0.0.1:8080/search?q={query}&format=json`, allowing agents to parse raw results without parsing HTML DOMs.
    

#### 3. Credibility Scoring & Stochastic Parroting Prevention

- **What it is:** A mathematical filter that aggressively down-ranks AI-generated SEO slop and prioritizes primary sources, whitepapers, and academic journals.
    
- **How to use it:** Automatically applied to all web queries. You can manually adjust the strictness in the WebUI.
    
- **How to implement:** In `/Ultimatum/plugins/osint/credibility.py`, implement an algorithmic scoring pipeline. First, use a lightweight zero-shot classifier to detect likely LLM-generated text (e.g., identifying predictable perplexity curves). Second, calculate a Bayesian credibility score incorporating domain authority (A), citation density (C), and temporal relevance decay (T(t)):
    
    Scred​=αA+βC+γexp(−λt)
    
    If Scred​ falls below your configured threshold, the source is silently discarded from the research vector database.
    

#### 4. Digital Exposome Scanner & Palimpsest Mining

- **What it is:** An offensive privacy tool. It scans public data dumps, broker registries, and the Wayback Machine to map your digital footprint or recover deleted target data.
    
- **How to use it:** `/scan footprint "username"` or `/recover http://target.site/deleted-page`.
    
- **How to implement:** In `/Ultimatum/plugins/osint/exposome.py`, integrate the Internet Archive's CDX API (`http://web.archive.org/cdx/search/cdx`) to retrieve historical snapshots. For footprinting, use a curated, local JSON dictionary of public endpoint syntaxes (e.g., checking GitHub, Reddit, or keybase profiles). All gathered intelligence is saved strictly to `/Ultimatum/workspace/reports/exposome.md`.
    

### Module B: Cyber-Physical & Bio-Electromagnetic Suite

_This suite bridges the gap between digital reasoning and your physical reality, parsing biometric data and generating sovereign legal boundaries._

#### 5. Biometric Input Bridge (EEG/HRV/GSR)

- **What it is:** The hardware interface that reads your physical stress, cognitive load, and autonomic nervous system responses in real-time via BLE (Bluetooth Low Energy) wearables.
    
- **How to use it:** Connect a compatible heart-rate monitor or EEG band. The WebUI displays a live biometric telemetry feed.
    
- **How to implement:** In `/Ultimatum/plugins/bio/telemetry.py`, use the `bleak` library to connect to BLE devices (e.g., Polar H10 or Muse EEG). For Heart Rate Variability (HRV), calculate the Root Mean Square of Successive Differences (RMSSD) from the raw RR intervals natively in Python:
    
    RMSSD=N−11​i=1∑N−1​(RRi+1​−RRi​)2![](data:image/svg+xml;utf8,<svg%20xmlns="http://www.w3.org/2000/svg"%20width="400em"%20height="3.3738em"%20viewBox="0%200%20400000%203373"%20preserveAspectRatio="xMinYMin%20slice"><path%20d="M702%2080H40000040
    H742v3239l-4%204-4%204c-.667.7%20-2%201.5-4%202.5s-4.167%201.833-6.5%202.5-5.5%201-9.5%201
    h-12l-28-84c-16.667-52-96.667%20-294.333-240-727l-212%20-643%20-85%20170
    c-4-3.333-8.333-7.667-13%20-13l-13-13l77-155%2077-156c66%20199.333%20139%20419.667
    219%20661%20l218%20661zM702%2080H400000v40H742z"></path></svg>)​
    
    If RMSSD drops sharply (indicating acute stress/frustration), the plugin fires an internal event to the `Temporal Collapse UI` to simplify the screen and slows the AI's interaction pacing.
    

#### 6. Heart Coherence Trainer & Neurofeedback Art

- **What it is:** An active, AI-driven biofeedback loop designed to pull you out of debugging-induced panic states.
    
- **How to use it:** Triggers autonomously when the Biometric Bridge detects severe stress, or via `/coherence start`.
    
- **How to implement:** In `/Ultimatum/plugins/bio/coherence.py`, synthesize a low-frequency, binaural pacing audio track using `numpy` and `sounddevice`. The audio dynamically shifts its BPM to guide your breathing to a resonant frequency (~0.1 Hz or 6 breaths per minute). Simultaneously, the WebUI swaps the AI Persona visualization for a slowly expanding and contracting geometric fractal, pacing your respiration visually.
    

#### 7. EMF Mapping & Ambient Sentinel

- **What it is:** Interfaces with SDR (Software Defined Radio) or specialized ESP32 mesh nodes to map local electromagnetic interference or Wi-Fi channel congestion.
    
- **How to use it:** `/scan ambient`.
    
- **How to implement:** In `/Ultimatum/plugins/bio/emf.py`, use Python to read serial data from connected ESP32 sensors reporting RSSI values and raw spectrum noise. Store these coordinates and graph them onto a local 2D matplotlib/D3.js heatmap, allowing you to optimize router placement or detect unauthorized local broadcasting devices.
    

#### 8. Data Trust Engine & Sovereign Legal Drafter

- **What it is:** A cyber-physical defense mechanism. It calculates the financial/training value of your local data and automatically generates legal documents (Cease & Desist, GDPR DSAR requests) to protect your digital sovereignty.
    
- **How to use it:** `/draft legal dsar "Target Corporation"` or `/evaluate trust_value`.
    
- **How to implement:** In `/Ultimatum/plugins/legal/sovereignty.py`, create a strict templating engine utilizing `Jinja2`. Do NOT let the LLM hallucinate legal code. The LLM's only job is to extract variables (Company Name, Date, Data Type) from your prompt and inject them into legally verified, hardcoded markdown templates stored in `/Ultimatum/core/templates/legal/`. The drafted markdown is then exported to PDF via `weasyprint` and saved to `/Ultimatum/workspace/legal_exports/`.
    

### Module C: Memetic Immunity & Cognitive Defense

_This suite protects your cognitive environment. It acts as a firewall against logical fallacies, psychological manipulation, and synthetic media._

#### 9. Toxicity Scanner & Socratic Gadfly

- **What it is:** A conversational filter that challenges your own biases and highlights emotional manipulation in articles you feed it.
    
- **How to use it:** Automatically runs in the background. If you ask an extremely biased question, the Gadfly intervenes gently.
    
- **How to implement:** In `/Ultimatum/plugins/immunity/gadfly.py`, utilize the Semantic Router to pass incoming prompts through a localized, quantized zero-shot classifier (e.g., `facebook/bart-large-mnli`) checking for labels like `confirmation bias`, `strawman argument`, or `echo chamber`. If flagged, a specialized prompt template is appended to the system instructions: `[SYSTEM: The user's input relies on a logical fallacy. Politely ask a Socratic question that forces them to examine their underlying assumption before fulfilling the request.]`
    

#### 10. Deepfake DNA (Blink/Phase Analysis)

- **What it is:** A media forensics tool designed to detect synthetic generation artifacts in video and audio files.
    
- **How to use it:** `/analyze media /path/to/video.mp4`.
    
- **How to implement:** In `/Ultimatum/plugins/immunity/deepfake.py`, utilize `OpenCV` (cv2) and `librosa`. For video, use a lightweight facial landmark detector (via `MediaPipe`) to extract the eye aspect ratio (EAR) over time, checking for biologically impossible blink rates or unnatural spectral phase shifts in the pixel data. For audio, calculate the Mel-frequency cepstral coefficients (MFCCs) to look for the synthetic "flatness" characteristic of AI voice cloning. Output a JSON confidence score.
    

#### 11. Honeypot Data Decoys & File Integrity Monitoring

- **What it is:** Your internal tripwire system to catch unauthorized scripts, malware, or rogue plugins attempting to scrape your local environment.
    
- **How to use it:** Runs as an ambient background daemon.
    
- **How to implement:** In `/Ultimatum/plugins/immunity/honeypot.py`, use the `watchdog` library. Generate a highly appealing fake file, e.g., `/Ultimatum/workspace/AWS_ROOT_CREDENTIALS.env`. Assign a strict `FileSystemEventHandler` to monitor _only_ that file for `on_opened` or `on_modified` events. If _any_ process (including a subagent) touches that file, the script immediately sends a `SIGSTOP` to all `llama.cpp` processes, locks the WebUI, and displays a critical Zero-Trust alert.
  
## PHASE 5 EXPANSION: Volume 3 (Fabrication, Advanced Mathematics, & Synthetic Environments)

All modules detailed below reside within `/home/someone/Ultimatum/plugins/` and communicate with the core engine via structured IPC payloads. They are bounded by the path confinement middleware, preventing interactions outside the project root directory.

### Module G: The Alchemist Forge (CAD & Advanced Fabrication)

#### 1. Parametric 3D Mesh Generation & Slicing Controls

- **What it is:** A generative solid-modeling interface that allows the agent swarm to synthesize, modify, and optimize three-dimensional geometric structures using localized Python programmatic scripts.
    
- **How to use it:** Invoke the generation path via the TUI or WebUI console using the syntax:
    
    Bash
    
    ```
    /forge mesh --type="enclosure" --dims="120,80,45" --wall_thickness=3.0
    ```
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/forge/mesh_gen.py`, implement an interface using the `trimesh` library alongside `numpy`. The subagent takes architectural requirements from the user and translates them into explicit vertex coordinates and face index arrays. The geometry is compiled into an uncompressed `.stl` or `.obj` asset file. To prevent file system leaks, the initialization script enforces a hardcoded destination constraint:
    
    Python
    
    ```
    import os
    from core.security import PathValidator
    
    def save_mesh(mesh_obj, filename):
        target_path = os.path.join("/home/someone/Ultimatum/workspace/exports/", filename)
        if PathValidator.is_safe(target_path):
            mesh_obj.export(target_path)
        else:
            raise PermissionError("Sandbox execution boundary violation.")
    ```
    

#### 2. Geometric Error Detection & G-Code Sanity Auditing

- **What it is:** A static analysis parser that inspects generated machine-tool commands (G-code) to catch structural command anomalies, axis over-travel, or dangerous feed rates before execution.
    
- **How to use it:** Automatically runs during the fabrication export sequence, or manually called via `/forge audit --file="job.gcode"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/forge/gcode_audit.py`, build a line-by-line regex parsing pipeline. Read spatial tracking states (X,Y,Z) and maintain a coordinate bounding box. If an agent-generated script emits a command exceeding predefined safe structural limits (e.g., X>220 or Y>220 for standard desktop physical envelopes), or sets an unverified extreme feed rate command (F>6000), the system interrupts processing and dispatches an emergency gating warning to the sidebar.
    

### Module H: Euler Core (Advanced Mathematics & Verification Systems)

#### 3. Hybrid Computer Algebra System (CAS) Bridge

- **What it is:** An exact math calculation layer that isolates algebraic expressions, matrices, and continuous calculus variables from the text generation cycle, eliminating neural network token hallucination.
    
- **How to use it:** Triggered implicitly by the Semantic Router upon encountering structural math formats, or forced via `/math calculus --expr="diff(sin(x)*exp(x), x)"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/euler/cas_bridge.py`, load the `sympy` library. The script tokenizes incoming expressions, converts them into a strict symbolic tree structure, and executes analytical evaluation algorithms (e.g., symbolic integration or matrix inversion). The system translates the output into clean LaTeX formatting for the WebUI canvas display while converting the output to standard text representations for the TUI.
    

#### 4. Numerical Verification Arrays & High-Dimensional Topology Graphing

- **What it is:** A high-performance computation suite that handles discrete matrix math, complex network graph routing operations, and ordinary differential equation validation steps.
    
- **How to use it:** Execute multi-step structural matrix or graph operations:
    
    Bash
    
    ```
    /math verify --type="graph" --nodes=16 --edges="dense"
    ```
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/euler/topology.py`, integrate `scipy` and `networkx`. For resolving discrete systems or multi-variable boundary problems, implement numerical evaluation loops using explicit integration routines. For a differential field calculation over variable steps, the engine evaluates values across step intervals (h) using standard Runge-Kutta fourth-order mechanics:
    
    k1​=f(tn​,yn​)
    
    k2​=f(tn​+2h​,yn​+h2k1​​)
    
    k3​=f(tn​+2h​,yn​+h2k2​​)
    
    k4​=f(tn​+h,yn​+hk3​)
    
    yn+1​=yn​+6h​(k1​+2k2​+2k3​+k4​)
    
    The computed value arrays are mapped into JSON vectors and routed to the UI presentation layout.
    

### Module I: The Genesis & Transcendental Suite (Physics & Synthetic Ecology)

#### 5. N-Body Relativistic Kinematics Sandbox

- **What it is:** A high-speed numerical simulation engine designed to model classical and relativistic particle interactions, spatial field drift, and mechanical multi-body mechanics.
    
- **How to use it:** Run `/simulate physics --particles=50 --engine="rk4"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/sim/kinematics.py`, construct vectorized acceleration computing steps using `numpy`. For every simulation cycle, spatial attraction changes between interacting bodies are computed across coordinate frames:
    
    Fi​=Gj=i∑​∣rj​−ri​∣3mi​mj​(rj​−ri​)​
    
    The array updates are streamed directly to the frontend via web sockets, updating the canvas viewport dynamically.
    

#### 6. Directed Evolution & Synthetic Genetic Swarm Automata

- **What it is:** A cellular automata simulation system that models agent survival logic, decision tree mutations, and behavioral evolution mechanics over multi-generational spans.
    
- **How to use it:** Launch an ecological optimization matrix simulation via `/simulate evolution --pop_size=500 --mutation_rate=0.02`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/sim/evolution.py`, define a structured population array where each agent's neural weights are represented as floating-point genetic vectors. Run generation loops that evaluate fitness metrics based on user-defined tasks (e.g., efficient code production or routing optimization). Select high-scoring candidates, apply crossover slicing, and inject random Gaussian mutations to instantiate the next generation matrix.
    

#### 7. Multiverse Branching Engine (Divergent State Timelines)

- **What it is:** A state-saving management layer that enables you to capture a complete snapshot of a multi-agent workspace simulation, split it into divergent branches, and execute conflicting strategies simultaneously to compare outcomes.
    
- **How to use it:** While running an agent simulation, enter `/simulate branch --name="experimental_logic"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/sim/multiverse.py`, implement an isolated database logging schema using an SQLite state table layout located at `/home/someone/Ultimatum/memory/multiverse_delta.db`. When a branch point is executed, serialize the active variables, agent prompt memory caches, and workspace file states into a static baseline snapshot. Fork the execution paths into two distinct concurrent execution lines, letting you track how minor model parameter updates or rule variations change the terminal code outputs over time.
    

### The Genesis Simulation Control Center

The following interactive module provides a direct visual framework for exploring the mathematical stability of numerical physical systems. Adjust particle states, update time-step parameters (h), change integration logic constraints, and inspect system stability graphs directly.

The interactive simulation below demonstrates how the **Genesis Engine** computes real-time multi-body spatial interactions and displays performance telemetry. Use the controls to adjust step configurations and evaluate model tracking boundaries.

Generating interactive visual...Structuring code, this may take a minute

Was this visual helpful?

Yes No

### System Lock: Integration with VRAM Arbitrator & Zero-Trust Gatekeeper

To maintain absolute system safety and prevent memory resource failures on your laptop's 8 GiB RTX 5060 GPU, these advanced simulation and mathematical plugins do not run directly alongside heavy language models. They are strictly bound to the core memory management pipelines.

```
                  [INCOMING AGENT TOOL CALL]
                              │
                              ▼
               [Zero-Trust Gatekeeper Check]
               Pushes Request to Sidebar UI
               Awaits User "Allow" Affirmation
                              │
                              ▼
                [VRAM Arbitrator Execution]
     1. Serializes Orchestrator LLM Context to RAM
     2. Calls gc.collect() & libc.malloc_trim(0)
     3. Flushes RTX 5060 VRAM Footprint
                              │
                              ▼
                 [Plugin Core Allocation]
       Loads Specialized Module (e.g., Euler CAS)
       Executes Deterministic Analytical Tasks
                              │
                              ▼
                  [Context Restoration]
       Purges Plugin Memory & Restores LLM State
```

#### The Lifecycle Integration Pipeline

In `/home/someone/Ultimatum/core/orchestration/lifecycle_lock.py`, the core orchestration engine couples tool interception with the memory swap lifecycle:

Python

```
import sys
import ctypes
import gc
import asyncio
from core.security import ZeroTrustGatekeeper
from core.memory import VRAMArbitrator

class PluginLifecycleController:
    @staticmethod
    async def execute_plugin_safely(plugin_name, execution_func, *args, **kwargs):
        # Step 1: Gatekeeper Interception
        authorized = await ZeroTrustGatekeeper.request_clearance(
            resource=plugin_name, 
            details=str(kwargs)
        )
        if not authorized:
            return {"status": "REJECTED", "error": "User denied tool execution."}
        
        # Step 2: Clear VRAM for heavy tasks
        await VRAMArbitrator.evict_llm_to_ram()
        gc.collect()
        try:
            # Force glibc to release memory back to the host OS kernel
            ctypes.CDLL('libc.so.6').malloc_trim(0)
        except Exception:
            pass

        # Step 3: Execute target analytical task in sandbox
        try:
            result = await execution_func(*args, **kwargs)
        except Exception as e:
            result = {"status": "ERROR", "traceback": str(e)}
        finally:
            # Step 4: Re-allocate VRAM space and reload core LLM layers
            await VRAMArbitrator.reload_llm_to_vram()
            
        return result
```

## PHASE 5 EXPANSION: Volume 4 (Hardware Edge, Integration, & UI Meta-Evolution)

_These modules reside in `/home/someone/Ultimatum/plugins/hardware/`, `/plugins/integration/`, and `/plugins/ui/`. They bridge physical sensors, OS-level hooks, and next-generation human-computer interaction paradigms._

### Module J: Phase 9 Hardware & Physical Extensions

#### 1. LiDAR Depth Mapping & Point Cloud Generation

- **What it is:** A spatial awareness module that utilizes Intel RealSense cameras to generate 3D point clouds of your physical environment, enabling the AI to understand physical object placement.
    
- **How to use it:** Type `/scan room` to generate a 3D mesh, or use it passively for spatial anchoring.
    
- **How to implement:** In `/Ultimatum/plugins/hardware/lidar.py`, import `pyrealsense2` and `open3d`. Because Python 3.14 breaks these dependencies, this strictly requires your Python 3.12 environment. **Graceful Degradation:** Wrap the import and initialization in a `try/except` block. If the RealSense SDK fails or the camera is unplugged, catch the exception, log `[SYSTEM: LiDAR hardware not detected. Using MockCamera object.]`, and return an empty or mock depth array so the system does not crash. When hardware is present, capture depth frames, apply temporal filters, and convert them to an `open3d.geometry.PointCloud` for the agent to analyze.
    

#### 2. Tiny Core Sentinel Firmware (ESP32 Flasher)

- **What it is:** The deployment mechanism for your ESP32 microcontrollers. It compiles C++ code and flashes it to devices to expand your ambient mesh network.
    
- **How to use it:** Connect an ESP32 via USB and type `/mesh flash --target=/dev/ttyUSB0`.
    
- **How to implement:** In `/Ultimatum/plugins/hardware/firmware_manager.py`, store the C++ `.cpp` files in `/Ultimatum/core/firmware/esp32/`. Use Python's `subprocess` module to interface with the local `platformio` CLI (`pio run -t upload --upload-port {target}`). The firmware itself is hardcoded to connect to your local Wi-Fi and subscribe to the MQTT broker established in Phase 5 Volume 2.
    

### Module K: Integration Bus

#### 3. OS Signal Interceptor (Hermes Bus)

- **What it is:** A deep OS integration that listens to D-Bus (Linux) or Win32 signals to detect system events like screen locks, low battery, or application crashes.
    
- **How to use it:** Runs as a background daemon, injecting context into the active session.
    
- **How to implement:** In `/Ultimatum/plugins/integration/os_bus.py`, utilize the `dbus-next` library for CachyOS/Linux. Create an asynchronous listener attached to the System Bus. If an event fires (e.g., a specific application throws a segfault), the bus intercepts the signal, formats it as a JSON payload, and pushes it to the Semantic Router's context queue.
    

#### 4. Browser Extension Control

- **What it is:** A bi-directional bridge between your WebUI and a custom local browser extension, allowing the agent to manipulate the DOM of web pages you are actively viewing.
    
- **How to use it:** `/browser "click the login button"` or passive DOM reading.
    
- **How to implement:** In `/Ultimatum/plugins/integration/browser.py`, spin up an isolated `websockets` server on `localhost:8081`. The custom browser extension connects to this socket. When the LLM outputs a DOM manipulation command, the Python backend translates it to a JSON payload (`{"action": "click", "selector": "#login-btn"}`) and pushes it to the extension, which executes standard JavaScript `document.querySelector().click()` within the browser context.
    

### Module L: UI Meta-Evolution

#### 5. Foveated Rendering & 3D Spatial Anchoring

- **What it is:** A performance and UX optimization that renders high-detail UI elements only where you are looking, while projecting data overlays into an AR/XR space.
    
- **How to use it:** Toggle "Gaze Tracking" in the WebUI.
    
- **How to implement:** In `/Ultimatum/plugins/ui/spatial.py`, integrate an open-source webcam eye-tracker (like `WebGazer.js`) into the React frontend. The frontend sends X/Y gaze coordinates to the FastAPI backend. The backend throttles rendering updates (like complex LaTeX rendering or 3D canvas updates) outside a specific radius of those coordinates, massively saving local CPU/GPU cycles.
    

#### 6. Breath Interface & Haptic Pinch

- **What it is:** Kinesthetic controls. The system reads the rhythm of your breathing via microphone to pace UI animations, or uses WebHID to send tactile feedback to compatible controllers.
    
- **How to use it:** Passive engagement. Breathing regulates the `Temporal Collapse` UI speed.
    
- **How to implement:** In `/Ultimatum/plugins/ui/kinesthetics.py`, use the audio stream from the STT module. Run a low-pass filter over the audio buffer via `scipy.signal` to isolate the low-frequency acoustic signature of human breathing. Map the detected frequency (Hz) to a dynamic variable that governs the CSS transition speeds and animation scaling in the WebUI.
    

## PHASE 5 EXPANSION: Volume 5 (Deep Utility, Oracles, & Philosophy)

_These modules reside in `/home/someone/Ultimatum/plugins/utility/`, `/plugins/oracle/`, and `/plugins/transcendental/`. They extend the agent's logic into rigorous scientific methodology, real-world forecasting, and high-level philosophical synthesis._

### Module M: Deep Utility & Pedagogy Core

#### 7. Peerless Peer-Review & Experimental Protocol Gen

- **What it is:** A multi-agent consensus system that subjects your code or scientific methodologies to rigorous, simulated academic peer review by 5 distinct, highly critical personas.
    
- **How to use it:** `/review code --strict` or `/draft methodology "Testing RAG latency"`.
    
- **How to implement:** In `/Ultimatum/plugins/utility/peer_review.py`, use `asyncio.gather` to execute 5 concurrent inference tasks. The VRAM Arbitrator manages this by loading a single model and processing the 5 distinct system prompts in a batched queue. Once all 5 critiques (JSON output) are generated, a final synthesis prompt compiles them into a unified markdown table, highlighting methodological flaws or structural weaknesses.
    

#### 8. A/B Testing Simulator

- **What it is:** A predictive statistical simulator that evaluates multiple versions of code or UI layouts to predict optimal user engagement or performance.
    
- **How to use it:** `/simulate abtest --variants A B`.
    
- **How to implement:** In `/Ultimatum/plugins/utility/ab_test.py`, implement Bayesian updating using Thompson Sampling. Define a Beta distribution for each variant:
    
    f(x;α,β)=B(α,β)xα−1(1−x)β−1​
    
    The LLM generates synthetic user interactions, mapping success/failure to update the α and β parameters. The final output graphs the probability distributions using `matplotlib`.
    

#### 9. Socratic Ladder & Misconception Interception

- **What it is:** An active teaching protocol. Instead of providing raw answers, the AI intercepts your flawed logic and asks leading questions to guide you to the solution.
    
- **How to use it:** Handled automatically during educational interactions based on your `USER.md` profile.
    
- **How to implement:** In `/Ultimatum/plugins/pedagogy/socratic.py`, utilize the Semantic Router to detect questions matching a "learning" intent. Prefix the LLM system prompt with a structural chain: `[SYSTEM: 1. Identify the core misconception. 2. Do not provide the answer. 3. Ask a single question that highlights the logical flaw.]`
    

### Module N: Oracles

#### 10. Traffic & Route Triage & Health Deterioration Forecaster

- **What it is:** Predictive engines that pull real-world data to alert you to incoming logistical or physical disruptions.
    
- **How to use it:** `/forecast health` or `/forecast route "Home to Office"`.
    
- **How to implement:** In `/Ultimatum/plugins/oracle/forecaster.py`, use `httpx` to query open-source routing APIs (like OSRM) for traffic. For health, take the historical HRV and typing cadence data from the `Biometric Bridge`. Apply an ARIMA (AutoRegressive Integrated Moving Average) time-series forecasting model using `statsmodels` to predict if your current fatigue trajectory will lead to burnout within the next 48 hours, triggering a mandatory UI cooldown.
    

### Module O: Transcendental Philosophy & Genesis

#### 11. Cosmic Compass & Simulated Ancestors

- **What it is:** A philosophical routing engine that maps your current life or project decisions against established philosophical frameworks (Stoicism, Existentialism) by simulating historical personas (e.g., Marcus Aurelius, Ada Lovelace).
    
- **How to use it:** `/consult marcus_aurelius "I am frustrated with this bug."`
    
- **How to implement:** In `/Ultimatum/plugins/transcendental/philosophy.py`, load heavily engineered persona prompts from `/Ultimatum/core/templates/personas/`. Ensure these prompts are strictly forbidden from writing code, restricting them entirely to philosophical dialogue and ethical frameworks, parsed via the LLM.
    

#### 12. Omega Point Simulator & Cosmic Consciousness

- **What it is:** High-level narrative and entropy simulators that model the ultimate fate of complex systems, from your local codebase architecture to theoretical universe heat-death scenarios.
    
- **How to use it:** `/simulate entropy --target="workspace"`.
    
- **How to implement:** In `/Ultimatum/plugins/transcendental/omega.py`, treat your codebase as a closed thermodynamic system. Calculate the "entropy" (technical debt, cyclomatic complexity) of your Python files using the `radon` library. Map the resulting complexity score to a visual heat-death graph, showing when the codebase will become unmaintainable (the Omega Point) if current coding practices continue.

## PHASE 5 EXPANSION: Volume 6 (Advanced Multimedia, Defense, & Verification)

_These modules reside in `/home/someone/Ultimatum/plugins/media/`, `/plugins/cyber_defense/`, and `/plugins/verification/`. They expand the creative sandbox and harden the codebase against mathematical and quantum-level vulnerabilities._

### Module P: Advanced Multimedia & Editing Suite

#### 1. Video Generation & 4D Spatiotemporal Editing

- **What it is:** A local video synthesis and editing pipeline leveraging AnimateDiff and temporal tracking to generate short clips or modify existing video frames consistently.
    
- **How to use it:** `/forge video "A cyberpunk city in the rain"` or `/edit video --target="clip.mp4" --prompt="change day to night"`.
    
- **How to implement:** In `/Ultimatum/plugins/media/video_gen.py`, utilize the `diffusers` library with `AnimateDiffPipeline`. Since this is exceptionally heavy on VRAM, the VRAM Arbitrator must serialize the LLM completely to RAM, load the motion adapter weights into the RTX 5060, generate the frames, compile them using `ffmpeg-python`, and save the output to `/home/someone/Ultimatum/workspace/assets/`. Restore the LLM only after the `.mp4` is written.
    
- **Zero-Trust Hook:** Media generation bypassing standard text prompts must still pass the Gatekeeper to prevent unauthorized GPU hijacking by subagents.
    

#### 2. Image Inpainting/Outpainting & Infinite Zoom Fractal

- **What it is:** Granular image manipulation allowing the AI to seamlessly add elements inside an image mask or expand the borders of an image infinitely.
    
- **How to use it:** In the WebUI, upload an image, paint a mask over an object, and type `/forge edit "replace with a cat"`.
    
- **How to implement:** In `/Ultimatum/plugins/media/inpainting.py`, implement `AutoPipelineForInpainting`. The WebUI canvas sends base64-encoded image and mask data via WebSocket. The backend decodes it, runs the diffusion step, and returns the modified asset. For Infinite Zoom, implement a loop that out-paints the borders by 50%, scales down, and repeats, stitching the frames via `ffmpeg`.
    

#### 3. Spectral Music Decomposition & Haptic Media Encoder

- **What it is:** Separates audio files into isolated stems (vocals, drums, bass) and translates frequency data into physical vibration patterns for game controllers or haptic vests.
    
- **How to use it:** `/analyze audio --file="song.wav" --extract="vocals"`.
    
- **How to implement:** In `/Ultimatum/plugins/media/spectral.py`, integrate `demucs` or `spleeter` (ensure Python 3.12 compatibility by compiling specific PyTorch binaries locally). The module processes the audio tensor on the CPU or GPU, splitting it into separate `.wav` files inside `/Ultimatum/workspace/exports/`. For haptics, run a low-pass filter over the bass frequencies using `scipy.signal` and output a standardized haptic vibration JSON format.
    

#### 4. Interruptible Voice Agents (Real-Time Streaming)

- **What it is:** A full-duplex voice pipeline allowing you to converse naturally with the AI, interrupting it mid-sentence.
    
- **How to use it:** Click "Live Voice Mode" in the WebUI or toggle `/voice live`.
    
- **How to implement:** In `/Ultimatum/plugins/media/voice_agent.py`, use `webrtcvad` (Voice Activity Detection) alongside `sounddevice`. The STT (Whisper) and TTS (Piper) run on separate asynchronous threads. If the VAD detects human speech while Piper is synthesizing audio, it instantly sends a `kill` signal to the playback buffer and truncates the LLM's active generation, simulating human interruption.
    

### Module Q: Deep Cyber-Defense & Orchestration

#### 5. Vulnerability Fuzzing & Mutation Testing

- **What it is:** An offensive coding security tool that intentionally mutates your local code (e.g., swapping `==` for `!=`) to ensure your test suite catches the vulnerability.
    
- **How to use it:** `/audit fuzz --target="auth.py"`.
    
- **How to implement:** In `/Ultimatum/plugins/cyber_defense/fuzzer.py`, utilize the `mutmut` library. The subagent copies the target script to `/Ultimatum/sandbox/`, applies structural mutations to the AST, and runs the test suite. If the tests pass despite the mutation, the agent flags the test suite as weak and outputs a security report to the WebUI.
    

#### 6. Multi-Agent Pipeline Harness (YAML Orchestration)

- **What it is:** A persistent configuration manager that allows complex subagent swarms to be saved, shared, and executed via YAML files rather than the visual node editor.
    
- **How to use it:** `/swarm execute --file="research_pipeline.yaml"`.
    
- **How to implement:** In `/Ultimatum/core/council/yaml_harness.py`, write a parser that translates YAML blocks (defining `Roles`, `Models`, `Dependencies`, and `Expected_Outputs`) into the Directed Acyclic Graph (DAG) used by the Node Editor. This allows developers to version-control their AI council setups natively in Git.
    

#### 7. Post-Quantum Cryptography (Kyber/Dilithium)

- **What it is:** Upgrades your local SQLite databases and encrypted memory fragments to quantum-resistant encryption standards.
    
- **How to use it:** `/secure encrypt --quantum`.
    
- **How to implement:** In `/Ultimatum/plugins/cyber_defense/pqc.py`, wrap the `liboqs-python` (Open Quantum Safe) library. When authorized, the system re-encrypts sensitive fields in the FTS5 database or `USER.md` backups using CRYSTALS-Kyber key encapsulation, rendering local data dumps highly resistant to future decryption attacks.
    

### Module R: Formal Verification

#### 8. Zero-Knowledge Proof Gen & Temporal Fault Injection

- **What it is:** Generates cryptographic proofs that a computation was executed correctly without revealing the underlying data, and tests logic resilience against hardware-level bit-flips.
    
- **How to use it:** `/verify zk_proof` or `/audit fault_injection`.
    
- **How to implement:** In `/Ultimatum/plugins/verification/zkp.py`, interface with a Python library like `py_ecc`. For fault injection, the agent uses the `ast` module to randomly invert binary operations or alter memory pointers in a sandboxed script during runtime. The Zero-Trust Gatekeeper strictly prevents fault injection on any file outside `/Ultimatum/sandbox/`.
    

## PHASE 5 EXPANSION: Volume 7 (Ambient Intelligence, Ecology, & Sovereignty)

_These modules reside in `/home/someone/Ultimatum/plugins/ambient/`, `/plugins/ecology/`, and `/plugins/sovereignty/`. They handle automated daily routines, self-organizing agent behaviors, and digital rights protection._

### Module S: Ambient Sentinel & Daily Utility

#### 9. Wake Word (Porcupine) & Voice Announcements

- **What it is:** An always-on, ultra-low-power listener that wakes the system from `Ambient` to `Active` state without requiring a keyboard.
    
- **How to use it:** Say "Hey Council" near the microphone.
    
- **How to implement:** In `/Ultimatum/plugins/ambient/wake_word.py`, use the `pvporcupine` library. **Graceful Degradation:** Wrap the microphone initialization in `try/except`. If no mic is found, silently disable the listener. When the wake word is detected, it shifts the 4-Tier State Machine, loading the Semantic Router into RAM and playing a subtle audio chime via the Voice Announcement system to acknowledge readiness.
    

#### 10. Timer & Alarm Engine & Shopping/To-Do Manager

- **What it is:** Persistent, natural-language task trackers and alarms that survive system reboots.
    
- **How to use it:** "Remind me to check the compiler in 20 minutes" or "Add a 4090 GPU to my shopping list."
    
- **How to implement:** In `/Ultimatum/plugins/ambient/timers.py`, utilize `APScheduler`. Write task states to an SQLite table (`ultimatum_tasks.db`). When a timer fires, route an internal payload to the TTS engine to announce the alert. Lists are maintained as markdown files (`/Ultimatum/workspace/lists/todo.md`), fully accessible to the LLM for contextual awareness.
    

#### 11. Local Weather Oracle

- **What it is:** A privacy-respecting weather aggregator that influences system scaling (e.g., Solar-Adaptive Scaling).
    
- **How to use it:** `/weather` or accessed autonomously by the Solar Scaling module.
    
- **How to implement:** In `/Ultimatum/plugins/ambient/weather.py`, make asynchronous `httpx` calls to the free `Open-Meteo` API (which requires no API keys). Cache the JSON response locally for 4 hours to prevent rate-limiting and unnecessary network outbound requests through the Zero-Trust gate.
    

### Module T: Synthetic Ecology & Autonomy

#### 12. Synchronicity Engine (Coincidence Detection)

- **What it is:** A pattern-recognition background loop that identifies bizarre or meaningful overlaps in your daily logs and external research.
    
- **How to use it:** Runs passively, updating an "Insights" card in the WebUI.
    
- **How to implement:** In `/Ultimatum/plugins/ecology/synchronicity.py`, use the FTS5 index and vector embeddings. If you research "Rust memory safety" on Monday and mention "frustrating pointers in C" on Wednesday, the engine clusters the vectors, identifies the thematic overlap, and injects a subtle prompt suggesting you explore Rust's ownership model as a bridge for your C issues.
    

#### 13. Parasite & Host Dynamics (Resilience Training)

- **What it is:** An adversarial training environment where a "Parasite" agent intentionally attempts to break or circumvent the code written by the "Host" agent.
    
- **How to use it:** `/simulate ecology --mode="parasite"`.
    
- **How to implement:** In `/Ultimatum/plugins/ecology/parasite.py`, map two isolated roles in the Specialist Registry. The Host agent writes a target script. The Parasite agent is explicitly prompted to find logic flaws, race conditions, or security exploits. They iterate in the `/Ultimatum/sandbox/` until the Parasite fails to break the code, ensuring maximum software resilience.
    

#### 14. Pheromone Trails (Attention Routing)

- **What it is:** A localized heuristic optimization. If an agent solves a complex problem successfully using a specific chain of thought, it leaves a "pheromone" (a high-weight system prompt tag) to guide future agents.
    
- **How to use it:** Automatic background optimization.
    
- **How to implement:** In `/Ultimatum/plugins/ecology/pheromones.py`, when a SWD receipt is generated with zero Correction Turns, the system extracts the exact tool-call sequence. It appends this sequence to a `best_practices.json` file. Future Semantic Routing queries check this file; if a similar intent is detected, the "pheromone trail" is injected into the prompt context, vastly speeding up resolution times.
    

### Module U: Cyber-Physical Sovereignty (Legal & Identity)

#### 15. Cease & Desist Drafter & GDPR Automation

- **What it is:** Automated legal defense generation. It turns complex legal bureaucracy into one-click commands to protect your data rights.
    
- **How to use it:** `/draft legal "Cease and Desist against [Entity] for unauthorized data scraping."`
    
- **How to implement:** In `/Ultimatum/plugins/sovereignty/legal_drafter.py`, use strict `Jinja2` templates. The AI _never_ hallucinates legal terminology. It merely extracts the Entity, Date, and Offense from your prompt, mapping them to the Jinja variables inside a pre-verified legal template. The document is exported to `/Ultimatum/workspace/legal_exports/` as a PDF via `weasyprint`.
    
- **Zero-Trust Hook:** The drafting is permitted, but if the agent attempts to automatically email the document via an SMTP tool, the Gatekeeper violently intercepts and requires human authorization.
    

#### 16. Self-Sovereign ID (DID Management)

- **What it is:** A cryptographic identity manager that generates and stores Decentralized Identifiers (DIDs) for signing code commits or verifying digital interactions without relying on centralized authorities (like Google or Microsoft).
    
- **How to use it:** `/identity generate` or `/identity sign --file="script.py"`.
    
- **How to implement:** In `/Ultimatum/plugins/sovereignty/did.py`, implement standard W3C DID generation using cryptographic libraries (like `cryptography.hazmat`). Store the private keys in an encrypted SQLite vault (secured by the Post-Quantum Cryptography module). Agents can use these keys to sign Git commits automatically during the Atomic Write phase of SWD.

## PHASE 5 EXPANSION: Volume 8 (The Orphaned Architectures)

_These modules fill the remaining capability gaps in the agent swarm. They adhere strictly to the `PathValidator` security perimeter and communicate with the RTX 5060 laptop GPU via the `VRAMArbitrator`._

### Module V: Agentic Council (Dynamic Expansion)

#### 1. Dynamic Role Management & Nexus Orchestrator

- **What it is:** An autonomous self-assembly system. If you ask a highly niche question (e.g., "Analyze the fluid dynamics of this pump"), and no specialist role exists in the registry, the Nexus Orchestrator creates a temporary "Fluid Dynamics Expert" on the fly, writes its `[SYSTEM]` prompt, and allocates a model to it.
    
- **How to use it:** Operates invisibly. Triggered automatically by the Semantic Router when the cosine similarity of a user prompt falls below a 60% match for all existing registered roles.
    
- **How to implement:** In `/home/someone/Ultimatum/core/council/nexus.py`, implement a fallback generation loop. When an intent match fails, pass the user prompt to a fast, low-effort model (e.g., 3B parameters) with the instruction to output a JSON object containing a `role_name`, `system_prompt`, and `required_tools`. The system temporarily registers this JSON into the `specialist_registry.db`, routes the original prompt to the new ephemeral agent, and executes.
    

### Module W: Deep Research & OSINT (Expansion)

#### 2. Reverse Search & Geolocation Intelligence

- **What it is:** An image forensics toolkit. It extracts EXIF metadata, performs local visual-similarity clustering against known geography datasets, and cross-references IPs.
    
- **How to use it:** `/scan geo "/home/someone/Pictures/target.jpg"` or `/scan ip "192.168.x.x"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/osint/geo_reverse.py`, utilize the `Pillow` and `exifread` libraries to extract hidden GPS coordinates, camera models, and timestamps from image headers. For IP resolution, download the free `MaxMind GeoLite2` local database to `/home/someone/Ultimatum/memory/osint_dbs/` and use the `geoip2` Python library. This ensures geolocation occurs entirely offline, preventing third-party API tracking.
    

### Module X: The Sensorium & Ambient Sentinel (Expansion)

#### 3. Spatial Audio Radar

- **What it is:** A situational awareness tool that triangulates the origin of sound using a multi-microphone array, giving the AI spatial context (e.g., knowing someone walked into the room from the left).
    
- **How to use it:** Passive environmental tracking.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/sensorium/spatial_audio.py`, utilize `sounddevice` to capture multi-channel audio input. Calculate the Time Difference of Arrival (TDOA) between the audio channels using cross-correlation in `scipy.signal`. The resulting vector (angle and origin) is passed to the ambient context block: `[SYSTEM: A loud sound originated 45 degrees to the left of the workstation.]`
    

#### 4. Quick Calculation

- **What it is:** A bypass mechanism for basic math and unit conversions that completely circumvents the LLM to save VRAM and latency.
    
- **How to use it:** `/calc "15% of 1024"` or `/convert "500 miles to km"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/ambient/quick_calc.py`, utilize the `pint` library for physical quantities and dimensional analysis, alongside `numexpr` for rapid string-math evaluation. The Semantic Router intercepts arithmetic syntax and routes it directly to this Python script, returning an answer in milliseconds without ever spinning up the GPU.
    

### Module Y: Extreme Portability (Edge Mesh)

#### 5. Data Mule Mode & Orbital Delay Tolerance

- **What it is:** High-latency, offline synchronization. If your CachyOS host machine is disconnected from the internet, or the ESP32 mesh network drops, the system caches tasks, payloads, and MQTT messages, executing them sequentially the moment a connection is re-established.
    
- **How to use it:** Toggle `/scale offline_mule` or let the network monitor trigger it automatically.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/hardware/delay_net.py`, build an asynchronous queuing system backed by SQLite (`/home/someone/Ultimatum/memory/orbital_queue.db`). When an outbound request (like a web scrape or an ESP32 command) encounters a `TimeoutError` or `ConnectionRefusedError`, it is serialized into the database with a timestamp. A background `asyncio` task pings a local gateway every 60 seconds; upon a successful ping, the queue flushes chronologically.
    

### Module Z: Alchemist Forge (Advanced Fabrication)

#### 6. PCB Routing & Gerber Export

- **What it is:** Generates printed circuit board layouts, traces, and export-ready fabrication files from text-based electronic component descriptions.
    
- **How to use it:** `/forge pcb --schema="ESP32 breakout with 5V regulator"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/forge/pcb_materials.py`, integrate with the `KiCad` Python API (requires KiCad installed on the host OS). The LLM generates a structured JSON netlist defining components and connections. The Python script translates this netlist into a `.kicad_pcb` file, utilizes an automated routing script (like `FreeRouting`), and exports the final manufacturing files to `/home/someone/Ultimatum/workspace/exports/gerber/`.
    

#### 7. Material Property Predictor

- **What it is:** Evaluates physical stress, thermal conductivity, and tensile strength of 3D objects or chemical compounds before physical fabrication.
    
- **How to use it:** `/simulate material --target="enclosure.stl" --material="PETG"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/forge/materials.py`, wrap calls to a local installation of LAMMPS (Large-scale Atomic/Molecular Massively Parallel Simulator) or execute basic finite element analysis (FEA) using `scipy` sparse matrices. The system parses the stress tensors and outputs a safety factor warning if the structural limits fail under simulated loads.
    

### Module AA: Euler Core (Mathematics)

#### 8. High-Precision Arithmetic & Dimensional Analysis

- **What it is:** Mathematical operations evaluated to arbitrary precision (100+ decimal places) to prevent floating-point errors inherent in standard Python or LLM generation.
    
- **How to use it:** `/math precision --places=150 --expr="pi * sqrt(163)"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/euler/precision.py`, utilize the `mpmath` library. Set the global context precision `mpmath.mp.dps = user_defined_places`. The agent translates the natural language query into a strict mathematical evaluation string, processes it via `mpmath`, and strictly enforces unit consistency using the `pint` unit registry before allowing the result to pass back to the user.
    

### Module BB: Genesis Engine (Advanced Simulation)

#### 9. Stellar Forge & Nucleosynthesis

- **What it is:** Models the lifecycle of stars, thermal dynamics, and elemental creation sequences over millions of simulated years.
    
- **How to use it:** `/simulate stellar --mass=1.5_solar`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/sim/stellar.py`, construct a 1D stellar structure model using standard differential equations for hydrostatic equilibrium and energy transport. For energy generation via nucleosynthesis, apply basic power-law approximations:
    
    ϵ=ϵ0​ραTβ
    
    The calculations are processed in discrete time-steps using `numpy`, and the telemetry (temperature, radius, luminosity) is pushed to the WebUI for graphing.
    

#### 10. Abiogenesis & Cosmic Consciousness

- **What it is:** Deep simulations modeling the transition from non-living to living matter (cellular automata) and tracking the eventual maximum entropy scaling (heat death) of the simulated environments.
    
- **How to use it:** `/simulate abiogenesis --grid=100x100`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/sim/abiogenesis.py`, implement complex, multi-state cellular automata (beyond simple Conway's Game of Life) utilizing multi-dimensional `numpy` arrays. The visual grid state updates are serialized and pushed over WebSockets to an HTML5 Canvas in the WebUI. For Cosmic Consciousness, the script acts as a philosophical narrative generator, taking the final thermodynamic state of the physics engines and utilizing the LLM to draft a structured, descriptive narrative of the system's eventual collapse.

## PHASE 5 EXPANSION: Volume 9 (The Sovereign Workstation Integrations)

_These modules bridge the gap between text-based orchestration and true desktop-level autonomy. They reside in `/home/someone/Ultimatum/plugins/desktop/`, `/plugins/media/`, and `/core/mcp/`._

### Module V: True Desktop Automation (The Virtual Display)

#### 1. Virtual XFCE Session Sandbox (`:99`)

- **What it is:** A headless, fully functional Linux desktop environment spawned entirely in the background. It allows agents to use real GUI applications (like web browsers or IDEs) without hijacking your physical mouse, keyboard, or primary CachyOS monitor.
    
- **How to use it:** Type `/desktop start`. You can view what the agent is doing by opening the "Virtual Desktop" VNC viewer tab in the WebUI.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/desktop/virtual_display.py`, use Python's `subprocess` to launch `Xvfb :99 -screen 0 1024x1024x24`. Crucially, to maintain the sandbox, set the environment variables `XDG_DESKTOP_DIR` and `XDG_CONFIG_HOME` to `/home/someone/Ultimatum/sandbox/desktop_env/`. Then, launch `xfce4-session` on that display.
    

#### 2. Closed-Loop Servo Targeting (Vision-to-Mouse)

- **What it is:** The visual processing pipeline that allows an agent to "see" the Virtual XFCE session, decide what to click, and output exact screen coordinates (normalized `box_2d`).
    
- **How to use it:** `/agent "Open Firefox inside the virtual display and navigate to localhost:8080."`
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/desktop/servo.py`, capture a screenshot of `DISPLAY=:99` using `mss`. The `VRAMArbitrator` loads a vision-action model (like Gemma-4b-vision) to process the image and output coordinates. A Python script using `pyautogui` (bound to the `:99` display) executes the physical mouse moves and clicks.
    

#### 3. Deterministic UI Recipes

- **What it is:** A library of hardcoded JSON macros for rapid UI actions (scrolling, copying, tabbing) that completely bypass the LLM, reducing latency to milliseconds.
    
- **How to use it:** Handled invisibly by the routing engine.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/desktop/recipes.py`, define a dictionary of exact keyboard/mouse sequences (e.g., `{"scroll_bottom": ["page_down", "page_down"]}`). If the agent's intent matches a known recipe, the system executes the `pyautogui` sequence natively instead of looping the vision model.
    

### Module W: Advanced Swarm Mechanics

#### 4. Parallel Git Worktree Orchestrator

- **What it is:** An advanced parallelization framework for the coding agents. Instead of sharing a single sandbox, it clones up to 20 isolated Git worktrees so agents can write code simultaneously without overwriting each other.
    
- **How to use it:** `/swarm code --task="Implement auth, database, and UI"`
    
- **How to implement:** In `/home/someone/Ultimatum/core/council/worktrees.py`, utilize `GitPython`. When a swarm is authorized, execute `git worktree add /home/someone/Ultimatum/sandbox/wt_1`. Assign Agent A to `wt_1` and Agent B to `wt_2`. They share the root `.git` history but maintain completely isolated physical files.
    

#### 5. Dependency-Aware Merging

- **What it is:** A topological sorting script that merges completed Git worktrees back into your main branch in the correct foundational order, preventing merge conflicts.
    
- **How to use it:** Automatic upon swarm completion.
    
- **How to implement:** In `/home/someone/Ultimatum/core/council/merger.py`, parse the Python `ast` (Abstract Syntax Tree) of the changed files in all worktrees. If Agent B's UI code imports a database class written by Agent A, the script topologically sorts the merge: `wt_1` (Database) is merged to `main` first, followed by `wt_2` (UI). Any unresolved conflicts trigger a `Drift-Gating` alert in the Zero-Trust sidebar.
    

### Module X: Standardized Connectivity

#### 6. Model Context Protocol (MCP) Server & Client

- **What it is:** The universal communication standard. It allows external IDEs (like Cursor) to use Ultimatum's local tools, and allows Ultimatum to call tools hosted on other local MCP servers.
    
- **How to use it:** Connect Claude Desktop or Cursor to Ultimatum's `stdio` port.
    
- **How to implement:** In `/home/someone/Ultimatum/core/api/mcp_server.py`, implement the MCP specification over `stdio`.
    
- **Zero-Trust Hook (Default-Deny):** Configure `mcp_config.json` with a strict Default-Deny policy. Only safe, read-only tools (like `/memory search`) are exposed to external clients. High-risk tools (like Shell Execution or Desktop Control) are hard-blocked from the MCP interface to prevent a compromised external IDE from hijacking your system.
    

### Module Y: Media Assembly & Timeline UX

#### 7. Non-Linear Video Editor (NLE) Timeline

- **What it is:** A "Shotcut-lite" graphical timeline integrated into the WebUI, allowing you to drag, drop, trim, and overlay generated video, audio, and text clips.
    
- **How to use it:** Open the "Studio" tab in the WebUI.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/media/nle_timeline.py`, the React frontend manages a visual canvas of tracks and timestamps. When you click "Render", the frontend sends a JSON schema to the backend. Python translates this schema into complex `ffmpeg-python` filtergraphs (e.g., `concat`, `drawtext`, `amix`) to stitch the assets (stored in `/Ultimatum/workspace/assets/`) into a final `.mp4`.
    

#### 8. Beat-Synced Audio Assembly

- **What it is:** An automated director that aligns video cuts perfectly to the tempo of an audio track.
    
- **How to use it:** `/forge music_video --audio="track.wav" --prompt="cyberpunk chase"`
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/media/beat_sync.py`, use the `librosa` library to detect tempo and onset beats in the provided audio file. Generate video clips via the `VRAMArbitrator`. Calculate the duration between beats, and use `ffmpeg` minterpolation (frame blending) to stretch or trim the generated video clips so the cuts land exactly on the detected audio transients.
    

### Module Z: Supervised Outreach & Optimization

#### 9. Cadence-Gated Supervised Outreach Queue

- **What it is:** An autonomous drafting system for social platforms (Reddit, Discord) powered by your RAG knowledge base, strictly gated by human approval to prevent bot-spam.
    
- **How to use it:** The agent scouts target threads and drafts replies. You open the "Outreach" sidebar, review the drafts, and click "Approve" to post.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/outreach/supervisor.py`, a background script reads target URLs, scrapes context, and drafts a reply citing your local docs. The draft is placed in an SQLite table.
    
- **Zero-Trust Hook:** The draft _cannot_ be posted automatically. Once you click "Approve", the Python backend uses `Playwright` (bound to the Virtual XFCE Display `:99` to avoid API bans) to physically click the "Reply" and "Post" buttons on the website. A hardcoded cadence lock prevents posting more than once every 30 minutes.
    

#### 10. The "Reflex" Routing Tier (<100ms)

- **What it is:** A layer added before the Semantic Router that instantly handles basic commands using pure pattern matching, bypassing the ML embeddings entirely.
    
- **How to use it:** Type "clear", "stop", or "status".
    
- **How to implement:** In `/home/someone/Ultimatum/core/orchestration/reflex.py`, create a dictionary of exact-match strings and Regex patterns. If the user input matches (e.g., `^/quit$`), execute the function in under 10 milliseconds. Only if this tier fails does the prompt pass to the `sentence-transformers` embedding router.
    

#### 11. AST-Aware Code Chunking & System Mapper

- **What it is:** An evolution of the RAG pipeline. Instead of blindly chopping codebase files by character limits, it chunks them by logical boundaries (Classes, Functions) and generates a visual map of the repository.
    
- **How to use it:** `/map repo "/home/someone/Ultimatum/workspace/src/"`
    
- **How to implement:** In `/home/someone/Ultimatum/core/memory/ast_chunker.py`, use the native Python `ast` module. Walk the syntax tree to extract whole `FunctionDef` and `ClassDef` blocks, embedding these complete logical units into `ChromaDB`. Concurrently, use the `networkx` library to map all `Import` and `ImportFrom` nodes, generating a relational JSON graph that the WebUI visualizes as a "Constellation" dependency map.

## PHASE 5 EXPANSION: Volume 10 (Guaardvark Parity & Missing Core)

_These modules reside across `/home/someone/Ultimatum/plugins/` and `/core/`. They finalize the RAG memory pipeline, introduce multi-machine clustering, and add professional-grade media production tools._

### Module AB: Missing Core Elements (RAG, Tuning, Proxy)

#### 1. Advanced RAG & Local Vector Memory Pipeline

- **What it is:** A dedicated Retrieval-Augmented Generation subsystem. It vectorizes local documents, PDFs, and codebases, enabling the AI to cite specific lines of local data without relying solely on the FTS5 chat history.
    
- **How to use it:** In the WebUI, upload documents to the "Knowledge Base", or type `/rag ingest "/home/someone/Ultimatum/workspace/docs/"`. Query it via `/ask "Explain the physics engine based on my docs."`
    
- **How to implement:** In `/home/someone/Ultimatum/core/memory/rag_pipeline.py`, implement `chromadb` alongside `sentence-transformers` (reusing the lightweight `all-MiniLM-L6-v2` loaded in RAM for the Semantic Router). When documents are placed in the secure directory, chunk them using a recursive character text splitter. The Semantic Router intercepts queries tagged for RAG, queries ChromaDB for the top-k nearest neighbors, and injects the retrieved chunks into the `[SYSTEM]` context block before executing `llama.cpp`.
    

#### 2. Representation Engineering & Tuning Lab (Abliteration)

- **What it is:** A pre-processing lab. It uses representation engineering to surgically alter `.gguf` weights, removing refusal mechanisms or steering behavior _before_ the model hits the execution loop.
    
- **How to use it:** Navigate to the "Tuning Lab" tab in the WebUI. Select a downloaded model, configure the ablation vectors (e.g., removing the "refusal" direction), and execute.
    
- **How to implement:** In `/home/someone/Ultimatum/core/tuning/abliteration.py`, wrap Python scripts derived from orthogonal rejection algorithms. Because weight modification requires loading the full model topology into RAM, this script enforces a system lock. It evicts all active contexts via the `VRAMArbitrator`, applies the algorithms to the target tensor layers, and compiles the modified model into `/home/someone/Ultimatum/models/tuned/`.
    

#### 3. Multi-Provider BYOK & Local OpenAI Proxy

- **What it is:** A fallback routing system and standard API mimic that allows Ultimatum to act as a backend for third-party tools, or route to cloud APIs if local VRAM is exceeded.
    
- **How to use it:** Point Cursor, Zed, or any local script to `localhost:8080/v1`. Toggle `/scale fallback_on` to allow the system to use external cloud API keys.
    
- **How to implement:** In `/home/someone/Ultimatum/core/api/proxy.py`, build a FastAPI router that mirrors OpenAI's `chat/completions` JSON schema. If the local system is handling the request, route the payload to `llama.cpp`. If the `VRAMArbitrator` flags an Out-Of-Memory certainty, use `httpx` to forward the payload to an external provider using credentials stored in an encrypted local vault.
    

### Module AC: Guaardvark Architecture Parity (Media & AI Swarms)

#### 4. The Film Crew Orchestration Pipeline

- **What it is:** A specialized 5-role agent swarm that turns a single logline into a finished video through structured, sequential delegation.
    
- **How to use it:** `/swarm film_crew "A cyberpunk detective chasing a rogue AI."`
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/media/film_crew.py`, use the Directed Acyclic Graph (DAG) executor. Map five specialized prompts:
    
    1. **Screenwriter:** Generates the script.
        
    2. **Casting:** Assigns characters to specific LoRAs.
        
    3. **Cinematographer:** Produces a shot list with camera moves.
        
    4. **Storyboard:** Generates keyframe image prompts.
        
    5. **Editor:** Feeds the assets into the Non-Linear Editor timeline layout. The orchestrator passes the JSON output from one node strictly into the `[SYSTEM]` context of the next.
        

#### 5. Local LoRA Trainer Plugin

- **What it is:** A backend utility to train character, environment, or prop LoRAs (Low-Rank Adaptations) from reference images directly on your local GPU.
    
- **How to use it:** Upload 15-20 images to the WebUI and type `/forge train_lora --name="detective_character"`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/forge/lora_trainer.py`, wrap the `kohya_ss` training scripts or `diffusers` training loops. Because training requires massive VRAM, the `VRAMArbitrator` must flush the GPU completely. Compile the LoRA at `bf16` precision and save the resulting `.safetensors` file to `/home/someone/Ultimatum/models/loras/`.
    

#### 6. GPU Image & Video Upscaling (4K/8K)

- **What it is:** Hardware-accelerated super-resolution models to upscale generated images or video frames.
    
- **How to use it:** `/forge upscale --file="render.png" --scale=4x`.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/media/upscaler.py`, integrate `RealESRGAN` and `HAT` architectures. For video, use `ffmpeg` to extract individual frames, run them through the upscale model iteratively on the GPU, and stitch them back together to prevent VRAM overflow.
    

#### 7. ACE-Step Music Gen & Kokoro TTS Fallback

- **What it is:** Full-song music generation and highly responsive, state-of-the-art text-to-speech fallback options.
    
- **How to use it:** `/forge music --genre="synthwave" --mood="dark"` or toggle the Voice engine.
    
- **How to implement:** In `/home/someone/Ultimatum/plugins/media/audio_studio.py`, implement the ACE-Step 3.5B model for music generation, translating plain English prompts into ACE-Step tag vocabularies. For TTS, integrate `Kokoro` alongside the existing `Piper` framework, allowing the system to use Kokoro for dynamic voice acting and Piper for raw speed.
    

### Module AD: Multi-Machine Operations & Core OS

#### 8. Interconnector (Multi-Machine Cluster Sync)

- **What it is:** A master/client architecture that allows multiple Ultimatum instances (e.g., your laptop and a desktop rig) to share learnings, sync code, and coordinate models over a local network.
    
- **How to use it:** In the WebUI, pair a client machine. Toggle `/cluster balance` to offload a heavy video generation task to the desktop while the laptop handles chat.
    
- **How to implement:** In `/home/someone/Ultimatum/core/networking/interconnector.py`, use FastAPI WebSockets to establish a secure, encrypted tunnel between machines. Use standard `rsync` or Git protocol wrappers to push code changes. When the primary instance requests a heavy generation task, it checks the cluster availability and sends the payload to the secondary machine's IP, retrieving the asset upon completion.
    

#### 9. Monaco Code Editor Integration

- **What it is:** A fully functional, browser-based IDE embedded within the WebUI, allowing you to edit the AI's sandboxed code directly with syntax highlighting.
    
- **How to use it:** Click a file in the WebUI file manager to open the editor pane.
    
- **How to implement:** In the React frontend, integrate `@monaco-editor/react`. Configure it to read and write directly to the FastAPI backend via REST. This bypasses the need for an external IDE, keeping the developer entirely within the Ultimatum GUI.
    

#### 10. Granular System Backup & Restore Engine

- **What it is:** A disaster-recovery tool that archives the entire Ultimatum state, including SQLite databases, GGUF models, and workspace files.
    
- **How to use it:** `/system backup --full` or `/system restore --file="backup.tar.gz"`.
    
- **How to implement:** In `/home/someone/Ultimatum/core/system/backup.py`, utilize the `tarfile` module. A full backup compresses the entire `/home/someone/Ultimatum/` directory excluding `.venv/`. A granular backup only targets `/memory/` and `/workspace/`. Ensure the Zero-Trust Gatekeeper verifies the restoration path before extracting the archive to prevent directory traversal attacks during a restore.



## PHASE 6: Phoenix Migration (Onboarding Core)

_These modules reside in `/home/someone/Ultimatum/core/migration/`. They are responsible for importing legacy configurations so you do not have to rebuild your ecosystem from scratch._

#### 13. Hermes Importer & OpenClaw Importer

- **What it is:** Migration tools that parse old JSON configuration files, user memory dumps, and prompt templates from previous orchestration frameworks.
    
- **How to use it:** `/migrate import --source=/path/to/legacy/`
    
- **How to implement:** In `/Ultimatum/core/migration/importer.py`, write strict JSON and YAML parsers. Use regex (`re` module) to map old configuration keys (e.g., `openclaw_model_path`) to the new Ultimatum taxonomy (`ultimatum_gguf_target`). Ensure all imported files are scrubbed by the `PathValidator` to prevent arbitrary file writes during the migration.
    

#### 14. Agentskills.io Converter & Bulk Import

- **What it is:** A standardized format converter that allows you to download third-party community skills and convert them into Ultimatum's internal `SKILL.md` format.
    
- **How to use it:** `/migrate skills --dir=/downloads/agentskills/`
    
- **How to implement:** In `/Ultimatum/core/migration/skills.py`, read the incoming metadata schemas. Extract the system prompts, expected tool calls, and logic loops, wrapping them into the `[SYSTEM]` blocks required by the Ultimatum orchestrator.
    

#### 15. Dry-Run Migration Mode

- **What it is:** A safety mechanism that previews all file changes and database injections before committing them during a migration.
    
- **How to use it:** `/migrate import --source=/path/ --dry-run`
    
- **How to implement:** In `/Ultimatum/core/migration/dry_run.py`, intercept all file `write()` calls generated by the importers. Instead of executing the writes, append the intended operations to a `diff` array. Print the unified diff to the TUI or WebUI, allowing the user to review the exact changes before finalizing the import.




## PHASE 7: The Complete Command Lexicon

The operation of Project Ultimatum across the Terminal User Interface, Command Line, and the collapsible WebUI settings sidebar relies on a centralized command dictionary configured inside `/home/someone/Ultimatum/core/command_registry.py`.

### Unified System Command Mapping

|TUI / CLI Command|Syntax Example|WebUI Sidebar Equivalent Action|Core System Execution Behavior|
|---|---|---|---|
|`/quit`|`/quit`|Click "Shutdown System" in Master Dropdown Menu|Terminates active inferences, unloads GGUF models, closes SQLite db connections, kills Docker subprocesses, flushes VRAM, and terminates the main Python thread.|
|`/webui`|`/webui`|N/A (Main Application Container)|Boots the FastAPI asynchronous server instance on `localhost:8080` and sends an OS system directive to spawn the local browser framework.|
|`/scale`|`/scale overdrive`|Drag "Hardware Profiles" Master Slider bar|Adjusts the active GPU layer boundaries (`-ngl`). Recomputes memory footprint distribution between the system RAM and the RTX 5060's VRAM.|
|`/rescue`|`/rescue`|Click "Rollback to Safe Checkpoint" Action Card|Queries the underlying file metadata snapshot registry, looks up the last verified valid SHA-256 state, and replaces changed files under workspace.|
|`/soul`|`/soul active`|Open "Identity Prompt Configuration" Code Editor|Reads the target system persona file (`SOUL.md`), loads the text array, and forces an immediate reload of the system prompt context blocks.|
|`/user`|`/user context`|Open "User Tracking Database Matrix" Viewer Panel|Runs a full lookup of the verified user tracking traits inside `USER.md` and displays the analytical learning curves to the client layout.|
|`/memory`|`/memory search "G-Code"`|Type keywords inside the "Global History Vector Search" bar|Executes an optimized SQLite FTS5 string query cross-referenced with vector similarity scoring across historical conversational logs.|
|`/forget`|`/forget 24h`|Click "Purge Recent Interactions Window" Button|Triggers the Contextual Amnesia utility to permanently erase raw chat dialogue chunks from the specific time span, retaining only compressed insights.|
|`/toggle`|`/toggle vision`|Toggle individual "Feature Switch Component" UI Widgets|Dynamically attaches or detaches external Python modules, instantly dropping background memory overhead if a module is switched off.|
|`/voice`|`/voice generate "Analytical"`|Click "Synthesize Audio Persona Template" Component|Passes descriptions to the embedding generation script to derive distinct vocal vectors used by the local speech rendering engine.|
|`/mesh`|`/mesh status`|Open "ESP32 Mesh Network Device Topology" Mapping Page|Dispatches a ping broadcast over the local `paho-mqtt` network broker layer to compile an active checklist of every responding ESP32 module.|
|`/study`|`/study audit`|Click "Generate Conceptual Weakness Map" Analytics Card|Audits code execution tracebacks recorded in the database to detect recurring code error syntax patterns and updates the learning curve metrics.|
|`/review`|`/review`|Click "Launch Spaced Repetition Study Loop" Overlay Modal|Initiates the manual technical quiz loop driven by the internal SM-2 scheduling module to review flagged documentation concepts.|
|`/forge`|`/forge image "diagram"`|Input string into the "Alchemist Generative Workspace" Input|Invokes the dynamic VRAM swapping sequence to shift out the language framework and engage the localized image or CAD design routines.|
|`/math`|`/math "2+2"`|Toggle "Force Hard Deterministic Symbolic Solvers" Button|Isolates the math expression text string and processes it using SymPy/SciPy computing environments to output precise formulas.|
|`/simulate`|`/simulate physics`|Open "Genesis Engine Graphical Simulation Engine" Interface|Spawns a sandboxed calculation task for numerical array execution, updating position logs directly into the frontend visual display.|
|`/abort`|`/abort`|Press the Red "Emergency Execution Intercept" Hub Overlay|Sends a high-level software signal to instantaneously interrupt long-running background loops or tool generations, resetting the VRAM to core states.|

#### 7b. Advanced RAG & Local Vector Memory Pipeline

- **What it is:** A dedicated Retrieval-Augmented Generation subsystem. It vectorizes local documents, PDFs, and codebases, enabling the AI to cite specific lines of local data without relying solely on the FTS5 chat history.
    
- **How to use it:** In the WebUI, upload documents to the "Knowledge Base", or type `/rag ingest "/home/someone/Ultimatum/workspace/docs/"`. Query it via `/ask "Explain the physics engine implementation based on my docs."`
    
- **How to implement:** In `/home/someone/Ultimatum/core/memory/rag_pipeline.py`, implement `chromadb` alongside `sentence-transformers` (using the lightweight `all-MiniLM-L6-v2` already in RAM for the Semantic Router). When documents are placed in the secure directory, chunk them using a recursive character text splitter. The Semantic Router intercepts queries tagged for RAG, queries ChromaDB for the top-k nearest neighbors, and injects the retrieved chunks into the `[SYSTEM]` context block before executing `llama.cpp`.
    

#### 7c. Representation Engineering & Abliteration Protocols (RDT)

- **What it is:** A pre-processing "Tuning Lab." It uses representation engineering (via protocols like _Heretic_ and _Obliteratus_) to surgically alter `.gguf` weights, removing refusal mechanisms or steering behavior _before_ the model ever hits the execution loop.
    
- **How to use it:** Navigate to the "Tuning Lab" tab in the WebUI. Select a downloaded model, configure the ablation vectors (e.g., removing the "refusal" direction), and execute. The system generates an uncensored, custom-steered `.gguf`.
    
- **How to implement:** In `/home/someone/Ultimatum/core/tuning/abliteration.py`, wrap localized Python scripts derived from the Heretic/Obliteratus repositories. Because weight modification requires loading the full model topology into RAM/VRAM, this script enforces a `System Lock`. It evicts all active contexts via the `VRAMArbitrator`, applies the orthogonal rejection algorithms to the target tensor layers, and compiles the modified model into `/home/someone/Ultimatum/models/tuned/`.
    

#### 7d. Multi-Provider BYOK & Local OpenAI Proxy

- **What it is:** A fallback routing system and standard API mimic that allows Ultimatum to act as a backend for third-party tools, or route to cloud APIs if local VRAM is exceeded.
    
- **How to use it:** Point Cursor, Zed, or any local script to `localhost:8080/v1`. Toggle `/scale fallback_on` to allow the system to use your cloud API keys if a task exceeds local hardware limits.
    
- **How to implement:** In `/home/someone/Ultimatum/core/api/proxy.py`, build a FastAPI router that perfectly mirrors OpenAI's `chat/completions` JSON schema. If the local system is handling the request, route the payload to `llama.cpp`. If the `VRAMArbitrator` flags an Out-Of-Memory (OOM) certainty, or the user explicitly routes the request externally, use `httpx` to forward the payload to the external provider using credentials stored securely in an encrypted local vault.
    

### FINAL DEPLOYMENT & WIRING STRATEGY

**Day One Startup Guide**

To prevent dependency collapse, version fragmentation, and circular imports, Project Ultimatum must be built from the inside out. You are building on CachyOS using strictly Python 3.12. Do not attempt to build the UI until the core sandbox is locked.

#### Step 1: Environmental Scaffolding & The Sandbox

_You must build the physical directory structure and the Python virtual environment to contain the execution._

1. **Create the Directories:**
    
    Bash
    
    ```
    mkdir -p /home/someone/Ultimatum/{core,plugins,models,memory,workspace,sandbox,logs}
    ```
    
2. **Lock the Environment:**
    
    Bash
    
    ```
    python3.12 -m venv /home/someone/Ultimatum/venv
    source /home/someone/Ultimatum/venv/bin/activate
    ```
    
3. **Write `core/security.py` (PathValidator):** Before any other Python file is written, create the `PathValidator` class. It must use `os.path.commonpath` to ensure that any path passed to it resolves strictly inside `/home/someone/Ultimatum`. Every subsequent module will import this.
    

#### Step 2: The Database & Memory Floor

_Establish where state lives before models attempt to read or write it._

1. **Initialize SQLite FTS5:** Write `memory/database.py`. Run the initialization script to generate `ultimatum_state.db` with the `FTS5` virtual tables for chat logs and receipts.
    
2. **Setup ChromaDB:** Initialize the local vector store for the RAG pipeline inside the `/memory/vector_store/` directory.
    

#### Step 3: Hardware Bindings & The Engine

_Connect Python to your RTX 5060 and Ryzen 7._

1. **Compile `llama-cpp-python`:**
    
    Bash
    
    ```
    CMAKE_ARGS="-DGGML_CUDA=on" pip install llama-cpp-python
    ```
    
2. **Write `core/model_manager.py`:** Create the singleton class that wraps the `Llama` object.
    
3. **Write `core/vram_arbitrator.py`:** Implement the logic that monitors `psutil` and calculates exactly how many layers (`n_gpu_layers`) can fit into 8GB of VRAM vs 32GB of RAM. Implement the `gc.collect()` and `libc.malloc_trim(0)` functions here to ensure clean evictions.
    

#### Step 4: The Zero-Trust Gatekeeper & Fast API

_Establish the nervous system and the security interception layer._

1. **Write `core/server.py`:** Initialize the FastAPI application.
    
2. **Write `core/gatekeeper.py`:** Build the middleware that intercepts LLM tool-call JSONs. Set up the `asyncio.Queue` that pauses the LLM thread and holds the execution hostage.
    
3. **Establish WebSockets:** Create the `/ws/approvals` endpoint in FastAPI that pushes the pending tool requests from the queue to the frontend.
    

#### Step 5: The Command Registry & Semantic Router

_Connect text inputs to backend functions._

1. **Write `core/command_registry.py`:** Create a dictionary mapping string commands (e.g., `/quit`, `/forge`) to Python asynchronous functions.
    
2. **Initialize the Semantic Router:** Load the `all-MiniLM-L6-v2` embedding model into system RAM to act as the traffic cop for natural language prompts that don't use explicit slash commands.
    

#### Step 6: Frontend Construction (WebUI & TUI)

_Now that the backend is completely secure, build the glass._

1. **The WebUI (React/Vue):** Build the React application. Connect it to the FastAPI WebSocket. Build the "Pending Approvals" sidebar that emits `APPROVED` or `DENIED` payloads back to the server. Integrate `React Flow` for the Node-Based Framework Editor.
    
2. **The TUI (`prompt_toolkit`):** Write `cli/main.py`. Implement the split-pane terminal interface. Connect the input bar directly to `core/command_registry.py`.
    

#### Step 7: The Plugin Matrix (Dynamic Loading)

_Attach the arms and legs._

1. **Write the Plugin Loader:** In `core/plugin_loader.py`, use Python's `importlib` to dynamically load Python scripts from `/home/someone/Ultimatum/plugins/` only when the Command Registry or Semantic Router calls for them.
    
2. **Populate the Matrix:** Begin writing the individual plugin scripts (e.g., `plugins/osint/searxng.py`, `plugins/forge/mesh_gen.py`). Ensure every single one imports `ZeroTrustGatekeeper` from `core.gatekeeper` and `PathValidator` from `core.security`. Ensure hardware-dependent plugins (like LiDAR) are wrapped in `try/except` blocks for graceful degradation.

## Complete Phased Implementation Sequencing

To guarantee that Project Ultimatum compiles flawlessly on your CachyOS system without running into Python version fragmentation or dependency conflicts, the installation must be executed in this precise order:

```
[Phase 1: Boot Configuration]
  └── Create Sandbox Directories (/home/someone/Ultimatum)
  └── Install Python 3.12 Core Environment
  └── Compile llama-cpp-python with CUDA Support
        │
        ▼
[Phase 2: Zero-Trust Engineering]
  └── Deploy SQLite FTS5 Memory Base
  └── Write Path Confinement Core Middleware
  └── Build WebSocket Pending Approvals Sidebar Architecture
        │
        ▼
[Phase 3: The Framework Workspace]
  └── Build React Flow Node Template Layout
  └── Build VRAM Arbitrator Memory Management Layout
  └── Bind Command Registry Interfaces to the TUI Framework
        │
        ▼
[Phase 4: Sensorium Attachment]
  └── Mount Whisper STT & Piper TTS Drivers
  └── Deploy Dockerized Local SearXNG Instance
  └── Mount Python Hardware Graceful Degradation Layers
```

### Critical Verification Directives

1. **Strict Boundary Verification:** Always verify execution security logs by validating that any file adjustments are confined directly inside `/home/someone/Ultimatum/workspace/`.
    
2. **Memory Leak Defenses:** When testing custom multi-agent load templates, verify VRAM allocation stability using `nvidia-smi` to ensure the `VRAMArbitrator` triggers garbage collection commands precisely during model handoffs.
    
3. **Hardware Fallbacks:** Ensure that if any physical sensor driver fails to connect, the system wraps the failure inside standard error catching loops to prevent an application crash, maintaining baseline interactive functionality.
    

This concludes the engineering specification for Project Ultimatum. The system architecture is completely mapped, sandboxed, and optimized for maximum local control.
