#include <on1x/on1x.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int write_text_file(const char* path, const char* text) {
    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        return 0;
    }
    const size_t length = strlen(text);
    const int ok = fwrite(text, 1, length, file) == length;
    fclose(file);
    if (!ok) {
        fprintf(stderr, "failed to write %s\n", path);
        return 0;
    }
    return 1;
}

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

static int ensure_dir(const char* path) {
    if (mkdir(path, 0755) == 0) return 1;
    if (errno == EEXIST) return 1;
    fprintf(stderr, "failed to create %s: %s\n", path, strerror(errno));
    return 0;
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

static int push_some_value(On1x_State* state) {
    return on1x_push_some(state) == ON1X_OK;
}

static int push_required_field(On1x_State* state, int table_index, const char* field_name) {
    if (on1x_push_tag(state, field_name, strlen(field_name)) != ON1X_OK) return 0;
    if (!on1x_table_get(state, table_index)) return 0;
    if (!on1x_is_some(state, -1)) return 0;
    if (!on1x_payload_of(state, -1)) return 0;
    if (!on1x_list_get(state, -1, 0)) return 0;
    return 1;
}

int main(void) {
    const char* script_path = ON1X_EXAMPLE_SCRIPTS_DIR "/incident_digest.on1x";
    const char* base_dir = "/tmp/on1x-example-incident";
    const char* logs_dir = "/tmp/on1x-example-incident/logs";
    const char* report_path = "/tmp/on1x-example-incident/out/incident-digest.md";
    const char* api_log =
        "2026-08-05T12:00:00Z INFO api request accepted\n"
        "2026-08-05T12:00:01Z WARN api p95 latency above threshold\n"
        "2026-08-05T12:00:02Z ERROR api upstream timeout\n";
    const char* worker_log =
        "2026-08-05T12:00:00Z INFO worker started batch\n"
        "2026-08-05T12:00:10Z INFO worker finished batch\n"
        "2026-08-05T12:00:11Z WARN worker retrying slow job\n";
    On1x_State* state = on1x_open();
    size_t script_length = 0;
    char* script_source = NULL;
    char* report_text = NULL;
    char run_expr[1024];

    if (!state) {
        fprintf(stderr, "failed to create On1x state\n");
        return 1;
    }

    if (!ensure_dir(base_dir) ||
        !ensure_dir(logs_dir) ||
        !ensure_dir("/tmp/on1x-example-incident/out") ||
        !write_text_file("/tmp/on1x-example-incident/logs/api.log", api_log) ||
        !write_text_file("/tmp/on1x-example-incident/logs/worker.log", worker_log)) {
        on1x_close(state);
        return 1;
    }

    on1x_grant(state, ON1X_CAP_FS);
    on1x_grant(state, ON1X_CAP_ENV);
    on1x_grant(state, ON1X_CAP_TIME);
    on1x_grant(state, ON1X_CAP_IO);
    if (on1x_open_std(state) != ON1X_OK ||
        on1x_open_fs(state) != ON1X_OK ||
        on1x_open_os(state) != ON1X_OK ||
        on1x_open_time(state) != ON1X_OK ||
        on1x_open_io(state) != ON1X_OK) {
        fprintf(stderr, "failed to install stdlib modules\n");
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
    snprintf(
        run_expr,
        sizeof(run_expr),
        "run(%%{ :InputDir => \"%s\", :OutputPath => \"%s\", :WriteFile => true })",
        logs_dir,
        report_path);
    if (on1x_eval(state, run_expr, strlen(run_expr), "<incident-run>") != ON1X_OK) {
        fprintf(stderr, "Run(config) failed\n");
        on1x_close(state);
        return 1;
    }

    {
        const int result_index = on1x_top(state);
        if (!push_required_field(state, result_index, "Files")) {
            fprintf(stderr, "result table is missing :Files\n");
            on1x_close(state);
            return 1;
        }
        printf("files scanned: %" PRId64 "\n", on1x_as_int(state, -1));
        on1x_pop(state, 3);

        if (!push_required_field(state, result_index, "FilesWithErrors")) {
            fprintf(stderr, "result table is missing :FilesWithErrors\n");
            on1x_close(state);
            return 1;
        }
        printf("files with ERROR: %" PRId64 "\n", on1x_as_int(state, -1));
        on1x_pop(state, 3);

        if (!push_required_field(state, result_index, "FilesWithWarnings")) {
            fprintf(stderr, "result table is missing :FilesWithWarnings\n");
            on1x_close(state);
            return 1;
        }
        printf("files with WARN: %" PRId64 "\n", on1x_as_int(state, -1));
        on1x_pop(state, 3);
    }

    report_text = read_text_file(report_path, NULL);
    if (!report_text) {
        fprintf(stderr, "failed to read generated report %s\n", report_path);
        on1x_close(state);
        return 1;
    }

    printf("\nGenerated report:\n%s\n", report_text);
    free(report_text);

    on1x_pop(state, 1);
    on1x_close(state);
    return 0;
}
