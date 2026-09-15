#ifndef WIZ8_TESTS_RUNTIME_AUDIO_SEMANTIC_TEST_H
#define WIZ8_TESTS_RUNTIME_AUDIO_SEMANTIC_TEST_H

struct AudioSemanticResult {
    unsigned char footstep_step_path;
    unsigned char footstep_jump_path;
    unsigned char footstep_scuff_path;
    unsigned char footstep_material_vocabulary;
    unsigned char footstep_bypass;
    unsigned char sound3d_ctor_registers;
    unsigned char sound3d_assign_registers_and_copies;
    unsigned char sound3d_dtor_removes;
    unsigned char ambient_serialize_roundtrip;
    unsigned char sound3d_falloff_volume;
    unsigned char mute_state_roundtrip;
};

bool RunAudioSemanticTests(AudioSemanticResult* result);
void PrintAudioSemanticResults(const AudioSemanticResult* result);

#endif
