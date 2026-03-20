use crate::state::UiState;
use egui::Ui;

pub fn show(ui: &mut Ui, state: &UiState) {
    ui.heading("ESP States");

    if state.last_signals.is_empty() {
        ui.label("No broadcast sent yet.");
        return;
    }

    let cols = 8;
    let cell_sz = egui::vec2(40.0, 40.0);
    let spacing = 6.0;

    egui::Grid::new("esp_grid")
        .spacing([spacing, spacing])
        .show(ui, |ui| {
            for (i, signal) in state.last_signals.iter().enumerate() {
                let color = if signal.state >= 0.9 {
                    egui::Color32::from_rgb(60, 180, 80) // on
                } else if signal.state >= 0.1 {
                    egui::Color32::from_rgb(200, 160, 40) // dim (future)
                } else {
                    egui::Color32::from_rgb(80, 80, 80) // off
                };

                // The returned value `resp` is used for tooltips, but we allocate a custom sized area
                let (rect, resp) = ui.allocate_exact_size(cell_sz, egui::Sense::hover());
                ui.painter().rect_filled(rect, 6.0, color);
                ui.painter().text(
                    rect.center(),
                    egui::Align2::CENTER_CENTER,
                    format!("{}", signal.id),
                    egui::FontId::proportional(11.0),
                    egui::Color32::WHITE,
                );

                // Tooltip on hover
                if resp.hovered() {
                    egui::show_tooltip_text(
                        ui.ctx(),
                        egui::Id::new(i),
                        format!("ESP {} — state: {:.1}", signal.id, signal.state),
                    );
                }

                if (i + 1) % cols == 0 {
                    ui.end_row();
                }
            }
        });
}
