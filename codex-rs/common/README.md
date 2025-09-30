# codex-common

This crate is designed for utilities that need to be shared across other crates in the workspace, but should not go in `core`.

For narrow utility features, the pattern is to add introduce a new feature under `[features]` in `Cargo.toml` and then gate it with `#[cfg]` in `lib.rs`, as appropriate.

## Model Presets

The `model_presets` module contains the definitions for AI models available in the `/model` menu in the Codex TUI.

### Adding New Models

To add a new model to the menu system:

1. Open `src/model_presets.rs`
2. Add a new `ModelPreset` entry to the `PRESETS` array
3. Follow the documentation in the module for required fields

See the module documentation in `model_presets.rs` for detailed examples and guidelines.

### Testing

The module includes comprehensive tests to ensure:
- All model presets have descriptions
- Descriptions follow consistent formatting
- Preset IDs are unique
- Expected model variants exist

Run tests with:
```bash
cargo test -p codex-common --all-features model_presets
```
