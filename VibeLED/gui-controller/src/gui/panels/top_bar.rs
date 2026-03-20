use crate::state::{Cmd, UiState};
use egui::Ui;
use tokio::sync::mpsc::Sender;

pub fn show(ui: &mut Ui, state: &UiState, cmd_tx: &Sender<Cmd>) {
    ui.horizontal_centered(|ui| {
        let btn = egui::Button::new(
            egui::RichText::new("⬛  CRITICAL STOP")
                .size(18.0)
                .color(egui::Color32::WHITE),
        )
        .fill(egui::Color32::from_rgb(200, 40, 40))
        .min_size(egui::vec2(220.0, 44.0));

        if ui.add(btn).clicked() {
            let _ = cmd_tx.try_send(Cmd::CriticalStop {
                total: state.total_esps,
                required: state.required_esps,
            });
        }

        ui.separator();
        ui.label(&state.status_text);

        if let Some(err) = &state.error {
            ui.colored_label(egui::Color32::RED, err);
        }
    });
}
