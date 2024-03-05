#include "scpi_commands.h"

// Close or open switches based on SCPI standard channel list.
// Examples:
//   ROUTE:CLOSE (@1,2)
//   ROUTE:OPEN (@1:4)
//   ROUTE:CLOSE? (@1)
scpi_result_t SCPI_ROUTe_OpenClose(scpi_t *context)
{
    scpi_parameter_t param;
    scpi_bool_t is_range = false;
    int32_t from_ch = 0, to_ch = 0;
    uint32_t channel_mask = 0;
    int index = 0;
    size_t dimensions;

    if (!SCPI_Parameter(context, &param, TRUE))
        return SCPI_RES_ERR;

    while (SCPI_ExprChannelListEntry(context, &param, index++, &is_range, &from_ch, &to_ch, 1, &dimensions) == SCPI_EXPR_OK)
    {
        if (from_ch < 1 || from_ch > RELAY_COUNT) return SCPI_RES_ERR;
        
        if (!is_range)
        {
            channel_mask |= (1 << (from_ch - 1));
        }
        else
        {
            if (to_ch < 1 || to_ch > RELAY_COUNT) return SCPI_RES_ERR;

            for (int i = from_ch; i <= to_ch; i++)
            {
                channel_mask |= (1 << (i - 1));
            }
        }
    }

    if (SCPI_CmdTag(context) == 0)
    {
        open_relays(channel_mask);
    }
    else if (SCPI_CmdTag(context) == 1)
    {
        close_relays(channel_mask);
    }
    else if (SCPI_CmdTag(context) == 2)
    {
        // Report status for each queried channel
        uint32_t state = relays_get_state();
        uint8_t result[RELAY_COUNT] = {0};
        int result_len = 0;
        for (int i = 0; i < RELAY_COUNT; i++)
        {
            if (channel_mask & (1 << i))
            {
                result[result_len++] = !!(state & (1 << i));
            }
        }

        SCPI_ResultArrayUInt8(context, result, result_len, SCPI_FORMAT_ASCII);
    }

    return SCPI_RES_OK;
}

scpi_result_t SCPI_ROUTe_OPENALL(scpi_t *context)
{
    open_relays(~0);
    return SCPI_RES_OK;
}

// Query list of all closed switches
scpi_result_t SCPI_ROUTe_STATEQ(scpi_t *context)
{
    uint32_t state = relays_get_state();
    char channel_list[3 + RELAY_COUNT * 2];
    channel_list[0] = '(';
    channel_list[1] = '@';
    int len = 2;
    
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        if (state & (1 << i))
        {
            if (len > 2) channel_list[len++] = ',';
            channel_list[len++] = '0' + (i + 1);
        }
    }

    channel_list[len++] = ')';

    SCPI_ResultArbitraryBlock(context, channel_list, len);
    return SCPI_RES_OK;
}

// Set state of relays from a numeric value
scpi_result_t SCPI_ROUTe_SET(scpi_t *context)
{
    uint32_t state = relays_get_state();
    uint32_t target;

    if (!SCPI_ParamUInt32(context, &target, true))
        return SCPI_RES_ERR;
    
    if (SCPI_CmdTag(context) == 0)
    {
        // Break before make
        open_relays(state & ~target);
        close_relays(target);
    }
    else
    {
        // Make before break
        close_relays(target);
        open_relays(state & ~target);
    }

    return SCPI_RES_OK;
}

scpi_result_t SCPI_ROUTe_GETQ(scpi_t *context)
{
    uint32_t state = relays_get_state();
    SCPI_ResultUInt32(context, state);
    return SCPI_RES_OK;
}

const scpi_command_t g_scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    { .pattern = "*CLS", .callback = SCPI_CoreCls,},
    { .pattern = "*ESE", .callback = SCPI_CoreEse,},
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
    { .pattern = "*OPC", .callback = SCPI_CoreOpc,},
    { .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
    { .pattern = "*RST", .callback = SCPI_CoreRst,},
    { .pattern = "*SRE", .callback = SCPI_CoreSre,},
    { .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
    { .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
    { .pattern = "*TST?", .callback = SCPI_CoreTstQ,},
    { .pattern = "*WAI", .callback = SCPI_CoreWai,},
    
    {"[ROUTe]:OPEN",            SCPI_ROUTe_OpenClose,   0},
    {"[ROUTe]:CLOSe",           SCPI_ROUTe_OpenClose,   1},
    {"[ROUTe]:CLOSe?",          SCPI_ROUTe_OpenClose,   2},
    {"[ROUTe]:OPEN:ALL",        SCPI_ROUTe_OPENALL,     0},
    {"[ROUTe]:CLOSe:STATe?",    SCPI_ROUTe_STATEQ,      0},
    {"[ROUTe]:SET[:BBM]",       SCPI_ROUTe_SET,         0},
    {"[ROUTe]:SET:MBB",         SCPI_ROUTe_SET,         1},
    {"[ROUTe]:GET?",            SCPI_ROUTe_GETQ,        0},
    
    SCPI_CMD_LIST_END
};

