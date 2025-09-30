//! Model presets for the Codex terminal application.
//!
//! This module defines the available AI models and their reasoning effort levels
//! that users can select from the `/model` menu in the TUI.
//!
//! # Extending the Model Menu System
//!
//! To add a new model preset to the menu:
//!
//! 1. Add a new `ModelPreset` entry to the `PRESETS` array in this file
//! 2. Provide a unique `id` (stable identifier used for configuration)
//! 3. Set a user-friendly `label` that will be displayed in the menu
//! 4. Write a clear `description` explaining when to use this model (start with "—" for consistency)
//! 5. Specify the `model` slug (e.g., "gpt-5", "gpt-5-codex")
//! 6. Set the reasoning `effort` level (Minimal, Low, Medium, or High)
//!
//! ## Example
//!
//! ```rust,ignore
//! ModelPreset {
//!     id: "my-model-medium",
//!     label: "my-model medium",
//!     description: "— balanced reasoning for general tasks",
//!     model: "my-model",
//!     effort: Some(ReasoningEffort::Medium),
//! }
//! ```
//!
//! The presets are displayed in the order they appear in the `PRESETS` array,
//! so consider organizing them by model family and reasoning effort for better UX.
//!
//! # Model Information
//!
//! For model-specific context window and token limits, see `codex-rs/core/src/openai_model_info.rs`.

use codex_core::protocol_config_types::ReasoningEffort;
use codex_protocol::mcp_protocol::AuthMode;

/// A simple preset pairing a model slug with a reasoning effort.
#[derive(Debug, Clone, Copy)]
pub struct ModelPreset {
    /// Stable identifier for the preset.
    pub id: &'static str,
    /// Display label shown in UIs.
    pub label: &'static str,
    /// Short human description shown next to the label in UIs.
    pub description: &'static str,
    /// Model slug (e.g., "gpt-5").
    pub model: &'static str,
    /// Reasoning effort to apply for this preset.
    pub effort: Option<ReasoningEffort>,
}

const PRESETS: &[ModelPreset] = &[
    ModelPreset {
        id: "gpt-5-codex-low",
        label: "gpt-5-codex low",
        description: "— optimized for coding tasks with some reasoning; balances speed and code quality for straightforward development work",
        model: "gpt-5-codex",
        effort: Some(ReasoningEffort::Low),
    },
    ModelPreset {
        id: "gpt-5-codex-medium",
        label: "gpt-5-codex medium",
        description: "— default coding model; provides strong reasoning for code generation, refactoring, and debugging tasks",
        model: "gpt-5-codex",
        effort: Some(ReasoningEffort::Medium),
    },
    ModelPreset {
        id: "gpt-5-codex-high",
        label: "gpt-5-codex high",
        description: "— maximizes code reasoning depth for complex architectures, system design, and advanced problem-solving",
        model: "gpt-5-codex",
        effort: Some(ReasoningEffort::High),
    },
    ModelPreset {
        id: "gpt-5-minimal",
        label: "gpt-5 minimal",
        description: "— fastest responses with limited reasoning; ideal for coding, instructions, or lightweight tasks",
        model: "gpt-5",
        effort: Some(ReasoningEffort::Minimal),
    },
    ModelPreset {
        id: "gpt-5-low",
        label: "gpt-5 low",
        description: "— balances speed with some reasoning; useful for straightforward queries and short explanations",
        model: "gpt-5",
        effort: Some(ReasoningEffort::Low),
    },
    ModelPreset {
        id: "gpt-5-medium",
        label: "gpt-5 medium",
        description: "— default setting; provides a solid balance of reasoning depth and latency for general-purpose tasks",
        model: "gpt-5",
        effort: Some(ReasoningEffort::Medium),
    },
    ModelPreset {
        id: "gpt-5-high",
        label: "gpt-5 high",
        description: "— maximizes reasoning depth for complex or ambiguous problems",
        model: "gpt-5",
        effort: Some(ReasoningEffort::High),
    },
];

pub fn builtin_model_presets(_auth_mode: Option<AuthMode>) -> Vec<ModelPreset> {
    PRESETS.to_vec()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_all_model_presets_have_descriptions() {
        let presets = builtin_model_presets(None);
        for preset in presets {
            assert!(
                !preset.description.is_empty(),
                "Model preset '{}' is missing a description",
                preset.id
            );
        }
    }

    #[test]
    fn test_gpt5_codex_presets_exist() {
        let presets = builtin_model_presets(None);
        let codex_presets: Vec<_> = presets
            .iter()
            .filter(|p| p.model == "gpt-5-codex")
            .collect();

        assert_eq!(
            codex_presets.len(),
            3,
            "Expected exactly 3 gpt-5-codex presets"
        );

        // Verify all three reasoning levels are present
        let has_low = codex_presets
            .iter()
            .any(|p| p.effort == Some(ReasoningEffort::Low));
        let has_medium = codex_presets
            .iter()
            .any(|p| p.effort == Some(ReasoningEffort::Medium));
        let has_high = codex_presets
            .iter()
            .any(|p| p.effort == Some(ReasoningEffort::High));

        assert!(has_low, "Missing gpt-5-codex low preset");
        assert!(has_medium, "Missing gpt-5-codex medium preset");
        assert!(has_high, "Missing gpt-5-codex high preset");
    }

    #[test]
    fn test_descriptions_start_with_em_dash() {
        let presets = builtin_model_presets(None);
        for preset in presets {
            if !preset.description.is_empty() {
                assert!(
                    preset.description.starts_with("—"),
                    "Description for '{}' should start with em-dash (—) for consistency",
                    preset.id
                );
            }
        }
    }

    #[test]
    fn test_preset_ids_are_unique() {
        let presets = builtin_model_presets(None);
        let mut ids = std::collections::HashSet::new();
        for preset in presets {
            assert!(
                ids.insert(preset.id),
                "Duplicate preset id found: '{}'",
                preset.id
            );
        }
    }
}
