use crate::state::{Cmd, UiState};
use egui::Ui;
use tokio::sync::mpsc::Sender;

pub fn show(ui: &mut Ui, state: &mut UiState, cmd_tx: &Sender<Cmd>) {
    ui.horizontal(|ui| {
        // Controls
        if state.playing {
            if ui.button("⏸").clicked() {
                let _ = cmd_tx.try_send(Cmd::Pause);
                state.playing = false; // Optimistic update
            }
        } else {
            if ui.button("▶").clicked() {
                let _ = cmd_tx.try_send(Cmd::Resume);
                state.playing = true; // Optimistic update
            }
        }
        if ui.button("⏹").clicked() {
            let _ = cmd_tx.try_send(Cmd::Stop);
        }

        ui.add_space(10.0);

        // Format time mm:ss
        let format_time = |ms: u64| {
            let secs = ms / 1000;
            let m = secs / 60;
            let s = secs % 60;
            format!("{:02}:{:02}", m, s)
        };

        ui.label(format_time(state.playback_pos_ms));

        // Slider for seek
        let mut pos = state.playback_pos_ms as f64;
        let max_pos = state.playback_dur_ms as f64;
        
        let slider = egui::Slider::new(&mut pos, 0.0..=max_pos.max(100.0))
            .show_value(false)
            .trailing_fill(true);
            
        let response = ui.add_sized(egui::vec2(250.0, 20.0), slider);
        if response.drag_stopped() || (response.changed() && ui.input(|i| i.pointer.any_click())) {
            let _ = cmd_tx.try_send(Cmd::Seek(pos as u64));
            state.playback_pos_ms = pos as u64; 
        }

        ui.label(format_time(state.playback_dur_ms));
        
        ui.add_space(20.0);
        if let Some(idx) = state.current_track {
            if let Some(t) = state.tracks.get(idx) {
                ui.label(egui::RichText::new(&t.name).strong());
            }
        }
    });
}
