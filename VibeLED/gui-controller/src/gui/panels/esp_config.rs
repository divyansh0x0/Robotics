use crate::state::{Cmd, UiState};
use egui::Ui;
use tokio::sync::mpsc::Sender;

pub fn show(ui: &mut Ui, state: &mut UiState, cmd_tx: &Sender<Cmd>) {
    ui.heading("ESP Config");
    ui.separator();

    ui.label("Total ESPs");
    if ui
        .add(egui::DragValue::new(&mut state.total_esps).speed(1.0).clamp_range(1..=255))
        .changed()
    {
        let _ = cmd_tx.try_send(Cmd::SetTotalEsps(state.total_esps));
    }

    ui.label("Required ON after stop");
    let max = state.total_esps;
    if ui
        .add(egui::DragValue::new(&mut state.required_esps).speed(1.0).clamp_range(0..=max))
        .changed()
    {
        let _ = cmd_tx.try_send(Cmd::SetRequiredEsps(state.required_esps));
    }

    ui.separator();
    ui.label(format!("Last broadcast: {} signals", state.last_signals.len()));
}
