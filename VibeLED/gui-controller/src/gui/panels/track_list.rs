use crate::state::{Cmd, UiState};
use egui::Ui;
use tokio::sync::mpsc::Sender;

pub fn show(ui: &mut Ui, state: &mut UiState, cmd_tx: &Sender<Cmd>) {
    ui.horizontal(|ui| {
        ui.heading("Tracks");
        if ui.button("📁 Folder").on_hover_text("Open folder").clicked() {
            if let Some(path) = rfd::FileDialog::new().pick_folder() {
                let _ = cmd_tx.try_send(Cmd::ChangeMusicFolder(path.to_string_lossy().to_string()));
            }
        }
        if ui.button("🎲").on_hover_text("Play random song").clicked() {
            let _ = cmd_tx.try_send(Cmd::PlayRandom);
        }
        if ui.button("⟳").on_hover_text("Refresh tracks").clicked() {
            let _ = cmd_tx.try_send(Cmd::RefreshTracks);
        }
    });

    ui.separator();

    egui::ScrollArea::vertical().show(ui, |ui| {
        for (i, track) in state.tracks.iter().enumerate() {
            let selected = state.current_track == Some(i);
            let label = egui::SelectableLabel::new(selected, &track.name);

            if ui.add(label).clicked() {
                state.current_track = Some(i);
                let _ = cmd_tx.try_send(Cmd::Play(i));
            }
        }
    });
}
