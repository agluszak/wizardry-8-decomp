// FUNCTION: WIZ8 0x0049BD00
float Det3_0049BD00(
    float param_1,
    float param_2,
    float param_3,
    float param_4,
    float param_5,
    float param_6,
    float param_7,
    float param_8,
    float param_9)
{
    return (param_2 * param_6 - param_3 * param_5) * param_7
        + ((param_5 * param_9 - param_6 * param_8) * param_1
            - (param_2 * param_9 - param_3 * param_8) * param_4);
}
