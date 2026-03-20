use crate::state::EspSignal;
use rand::seq::SliceRandom;

/// Randomly assigns ON/OFF states to all ESPs.
/// Exactly `required` ESPs get state=1.0, the rest get state=0.0.
pub fn random_assign(total: u32, required: u32) -> Vec<EspSignal> {
    let required = required.min(total) as usize;

    let mut ids: Vec<u32> = (0..total).collect();
    ids.shuffle(&mut rand::thread_rng());

    ids.iter()
       .enumerate()
       .map(|(i, &id)| EspSignal {
           id,
           state: if i < required { 1.0 } else { 0.0 },
       })
       .collect()
}
