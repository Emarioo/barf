#pragma once

typedef struct BarfObject BarfObject;


int register_jit_debug(void* obj_blob, int obj_bob_size);

void unregister_jit_debug(int entry_id);

void* create_memory_obj_from_artifact(BarfObject* object, int* out_obj_size);
