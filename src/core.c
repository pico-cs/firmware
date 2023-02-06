#include "core.h"

// command
static const int core_num_entry_queue  = 1;
static const bool core_dummy_data = true; 

void core_init(core_t *core) {
    queue_init(&core->q0, sizeof(bool), core_num_entry_queue);
    queue_init(&core->q1, sizeof(bool), core_num_entry_queue);
}

void core_signal_start1(core_t *core) {
    bool data = true;
    queue_add_blocking(&core->q0, &data);
}

void core_wait_start1(core_t *core) {
    bool data = true;
    queue_remove_blocking(&core->q0, &data);
}

bool core_stop1(core_t *core) {
    bool data;
    if (!queue_try_remove(&core->q1, &data)) return false;
    queue_add_blocking(&core->q0, &data);
    return true;
}

void core_wait_stop1(core_t *core) {
    bool data = true;
    queue_add_blocking(&core->q1, &data);
    queue_remove_blocking(&core->q0, &data);
}
