
<p align="center"><code>npm i -g @zfifteen/grok</code></p>

<p align="center"><strong>Grok CLI</strong> is a coding agent that runs locally on your computer and supports multiple AI providers including xAI and OpenAI.</p>

<p align="center">
  <img src="./.github/codex-cli-splash.png" alt="Grok CLI splash" width="80%" />
  </p>

---

## Quickstart

### Installing and running Grok CLI

Install globally with npm:

```shell
npm install -g @zfifteen/grok
```

Then simply run `grok` to get started:

```shell
grok
```

<details>
<summary>You can also go to the <a href="https://github.com/zfifteen/Grok-Codex/releases/latest">latest GitHub Release</a> and download the appropriate binary for your platform.</summary>

Each GitHub Release contains many executables, but in practice, you likely want one of these:

- macOS
  - Apple Silicon/arm64: `codex-aarch64-apple-darwin.tar.gz`
  - x86_64 (older Mac hardware): `codex-x86_64-apple-darwin.tar.gz`
- Linux
  - x86_64: `codex-x86_64-unknown-linux-musl.tar.gz`
  - arm64: `codex-aarch64-unknown-linux-musl.tar.gz`

Each archive contains a single entry with the platform baked into the name (e.g., `grok-x86_64-unknown-linux-musl`), so you likely want to rename it to `grok` after extracting it.

</details>

### Authentication and Provider Configuration

Grok CLI supports multiple AI providers:

- **xAI (Grok)**: Use xAI's Grok models with `XAI_API_KEY` environment variable. See [xAI Configuration Guide](./docs/xai_configuration.md) for setup instructions.
- **OpenAI**: Use OpenAI models via ChatGPT login or API key. See [Authentication](./docs/authentication.md) for details.

Run `grok` and follow the authentication prompts, or configure your preferred provider in `~/.codex/config.toml`.

### Model Context Protocol (MCP)

Grok CLI supports [MCP servers](./docs/advanced.md#model-context-protocol-mcp). Enable by adding an `mcp_servers` section to your `~/.codex/config.toml`.


### Configuration

Grok CLI supports a rich set of configuration options, with preferences stored in `~/.codex/config.toml`. For full configuration options, see [Configuration](./docs/config.md).

For xAI-specific configuration, see the [xAI Configuration Guide](./docs/xai_configuration.md).

---

### Docs & FAQ

- [**Getting started**](./docs/getting-started.md)
  - [CLI usage](./docs/getting-started.md#cli-usage)
  - [Running with a prompt as input](./docs/getting-started.md#running-with-a-prompt-as-input)
  - [Example prompts](./docs/getting-started.md#example-prompts)
  - [Memory with AGENTS.md](./docs/getting-started.md#memory-with-agentsmd)
  - [Configuration](./docs/config.md)
- [**xAI Configuration**](./docs/xai_configuration.md) - Configure Grok CLI to use xAI's Grok models
- [**Sandbox & approvals**](./docs/sandbox.md)
- [**Authentication**](./docs/authentication.md)
  - [Auth methods](./docs/authentication.md#forcing-a-specific-auth-method-advanced)
  - [Login on a "Headless" machine](./docs/authentication.md#connecting-on-a-headless-machine)
- [**Advanced**](./docs/advanced.md)
  - [Non-interactive / CI mode](./docs/advanced.md#non-interactive--ci-mode)
  - [Tracing / verbose logging](./docs/advanced.md#tracing--verbose-logging)
  - [Model Context Protocol (MCP)](./docs/advanced.md#model-context-protocol-mcp)
- [**Contributing**](./docs/contributing.md)
- [**Install & build**](./docs/install.md)
  - [System Requirements](./docs/install.md#system-requirements)
  - [DotSlash](./docs/install.md#dotslash)
  - [Build from source](./docs/install.md#build-from-source)
- [**FAQ**](./docs/faq.md)
- [**Open source fund**](./docs/open-source-fund.md)

---

## License

This repository is licensed under the [Apache-2.0 License](LICENSE).
