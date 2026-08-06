#include <on1x/on1x.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_text_file(const char* path, size_t* length_out) {
    FILE* file = fopen(path, "rb");
    char* buffer;
    long size;

    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = (char*)malloc((size_t)size + 1U);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[size] = '\0';
    fclose(file);
    if (length_out) *length_out = (size_t)size;
    return buffer;
}

static int push_table_field_int(On1x_State* state, int table_index, const char* key, int64_t value) {
    return on1x_push_tag(state, key, strlen(key)) == ON1X_OK &&
        (on1x_push_int(state, value), 1) &&
        on1x_table_set(state, table_index) == ON1X_OK;
}

static int push_table_field_bool(On1x_State* state, int table_index, const char* key, int value) {
    return on1x_push_tag(state, key, strlen(key)) == ON1X_OK &&
        (on1x_push_bool(state, value), 1) &&
        on1x_table_set(state, table_index) == ON1X_OK;
}

static int push_table_field_string(On1x_State* state, int table_index, const char* key, const char* value) {
    return on1x_push_tag(state, key, strlen(key)) == ON1X_OK &&
        on1x_push_string(state, value, strlen(value)) == ON1X_OK &&
        on1x_table_set(state, table_index) == ON1X_OK;
}

static int push_required_field(On1x_State* state, int table_index, const char* field_name) {
    if (on1x_push_tag(state, field_name, strlen(field_name)) != ON1X_OK) return 0;
    if (!on1x_table_get(state, table_index)) return 0;
    if (!on1x_is_some(state, -1)) return 0;
    if (!on1x_payload_of(state, -1)) return 0;
    if (!on1x_list_get(state, -1, 0)) return 0;
    return 1;
}

static On1x_Status host_metric(On1x_State* state, int argc) {
    size_t metric_length = 0;
    size_t service_length = 0;
    const char* metric_name;
    const char* service_name;

    if (argc != 2) return on1x_error(state, "Host.Metric expects metric name and service name");

    metric_name = on1x_as_string(state, 1, &metric_length);
    service_name = on1x_as_string(state, 2, &service_length);
    if (!metric_name || !service_name) {
        return on1x_error(state, "Host.Metric expects String arguments");
    }

    if (metric_length == strlen("error_budget_remaining") &&
        strncmp(metric_name, "error_budget_remaining", metric_length) == 0) {
        int64_t budget = 70;
        if (service_length == strlen("checkout") && strncmp(service_name, "checkout", service_length) == 0) {
            budget = 82;
        } else if (service_length == strlen("billing") && strncmp(service_name, "billing", service_length) == 0) {
            budget = 18;
        }
        on1x_push_int(state, budget);
        return on1x_push_some(state);
    }

    on1x_push_none(state);
    return ON1X_OK;
}

static On1x_Status host_record_decision(On1x_State* state, int argc) {
    size_t service_length = 0;
    size_t action_length = 0;
    const char* service_name;
    const char* action_name;

    if (argc != 3) return on1x_error(state, "Host.RecordDecision expects service, action, and score");

    service_name = on1x_as_string(state, 1, &service_length);
    action_name = on1x_as_string(state, 2, &action_length);
    if (!service_name || !action_name) {
        return on1x_error(state, "Host.RecordDecision expects String, String, Int");
    }

    printf(
        "host audit: service=%.*s action=%.*s score=%" PRId64 "\n",
        (int)service_length,
        service_name,
        (int)action_length,
        action_name,
        on1x_as_int(state, 3));
    on1x_push_unit(state);
    return ON1X_OK;
}

static int call_decide_and_print(On1x_State* state, const char* service, int64_t latency, int64_t error_rate, int64_t cpu, int has_incident) {
    char expr[1024];
    snprintf(
        expr,
        sizeof(expr),
        "decide(%%{ :Service => \"%s\", :P95LatencyMs => %" PRId64 ", :ErrorRatePct => %" PRId64 ", :CpuSaturationPct => %" PRId64 ", :HasActiveIncident => %s })",
        service,
        latency,
        error_rate,
        cpu,
        has_incident ? "true" : "false");

    if (on1x_eval(state, expr, strlen(expr), "<rollout-decide>") != ON1X_OK) {
        fprintf(stderr, "Decide(context) failed\n");
        return 0;
    }

    {
        const int result_index = on1x_top(state);
        int payload_index = 0;
        if (!on1x_tag_of(state, result_index) || !on1x_is_some(state, -1) || !on1x_payload_of(state, -1) || !on1x_list_get(state, -1, 0)) {
            fprintf(stderr, "decision value is not a tagged list\n");
            return 0;
        }

        {
            size_t tag_length = 0;
            const char* tag_name = on1x_as_string(state, -1, &tag_length);
            printf("%s -> %.*s\n", service, (int)tag_length, tag_name ? tag_name : "<unknown>");
        }
        on1x_pop(state, 3);

        if (!on1x_payload_of(state, result_index) || !on1x_list_get(state, -1, 0)) {
            fprintf(stderr, "decision payload is missing\n");
            return 0;
        }
        payload_index = on1x_top(state);

        if (!push_required_field(state, payload_index, "Reason")) {
            fprintf(stderr, "decision payload is missing :Reason\n");
            return 0;
        }
        {
            size_t reason_length = 0;
            const char* reason = on1x_as_string(state, -1, &reason_length);
            printf("  reason: %.*s\n", (int)reason_length, reason ? reason : "<unknown>");
        }
        on1x_pop(state, 3);

        if (!push_required_field(state, payload_index, "Score")) {
            fprintf(stderr, "decision payload is missing :Score\n");
            return 0;
        }
        printf("  score: %" PRId64 "\n", on1x_as_int(state, -1));
        on1x_pop(state, 3);

        on1x_pop(state, 2);
    }

    on1x_pop(state, 1);
    return 1;
}

int main(void) {
    static const On1x_FnDesc host_functions[] = {
        {"Metric", host_metric},
        {"RecordDecision", host_record_decision}
    };
    static const On1x_ModuleDesc host_module = {
        "Host",
        ON1X_CAP_NONE,
        host_functions,
        sizeof(host_functions) / sizeof(host_functions[0])
    };
    const char* script_path = ON1X_EXAMPLE_SCRIPTS_DIR "/rollout_policy.on1x";
    On1x_State* state = on1x_open();
    size_t script_length = 0;
    char* script_source = NULL;
    if (!state) {
        fprintf(stderr, "failed to create On1x state\n");
        return 1;
    }

    if (on1x_open_std(state) != ON1X_OK || on1x_install_module(state, &host_module) != ON1X_OK) {
        fprintf(stderr, "failed to prepare On1x host environment\n");
        on1x_close(state);
        return 1;
    }

    script_source = read_text_file(script_path, &script_length);
    if (!script_source) {
        fprintf(stderr, "failed to read %s\n", script_path);
        on1x_close(state);
        return 1;
    }

    if (on1x_eval(state, script_source, script_length, script_path) != ON1X_OK) {
        fprintf(stderr, "script evaluation failed\n");
        free(script_source);
        on1x_close(state);
        return 1;
    }
    free(script_source);

    on1x_pop(state, 1);
    if (!call_decide_and_print(state, "checkout", 185, 0, 62, 0) ||
        !call_decide_and_print(state, "billing", 460, 3, 91, 1)) {
        on1x_close(state);
        return 1;
    }

    on1x_close(state);
    return 0;
}
