#include <string.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/input_state.h"
#include "sway/workspace.h"
#include "list.h"
#include "log.h"
#include "stringop.h"

struct cmd_results *cmd_workspace(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "workspace", EXPECTED_AT_LEAST, 1))) {
		return error;
	}
	if (argc == 1 || (argc >= 2 && strcasecmp(argv[0], "number") == 0) ) {
		if (config->reading || !config->active) {
			return cmd_results_new(CMD_DEFER, "workspace", NULL);
		}
		swayc_t *ws = NULL;
		if (argc >= 2) {
			if (!(ws = workspace_by_number(argv[1]))) {
				char *name = join_args(argv + 1, argc - 1);
				ws = workspace_create(name);
				free(name);
			}
		} else if (strcasecmp(argv[0], "next") == 0) {
			ws = workspace_next();
		} else if (strcasecmp(argv[0], "prev") == 0) {
			ws = workspace_prev();
		} else if (strcasecmp(argv[0], "next_on_output") == 0) {
			ws = workspace_output_next();
		} else if (strcasecmp(argv[0], "prev_on_output") == 0) {
			ws = workspace_output_prev();
		} else if (strcasecmp(argv[0], "back_and_forth") == 0) {
			// if auto_back_and_forth is enabled, workspace_switch will swap
			// the workspaces. If we created prev_workspace here, workspace_switch
			// would put us back on original workspace.
			if (config->auto_back_and_forth) {
				ws = swayc_active_workspace();
			} else if (prev_workspace_name && !(ws = workspace_by_name(prev_workspace_name))) {
				ws = workspace_create(prev_workspace_name);
			}
		} else {
			if (!(ws = workspace_by_name(argv[0]))) {
				ws = workspace_create(argv[0]);
			}
		}
		swayc_t *old_output = swayc_active_output();
		workspace_switch(ws);
		swayc_t *new_output = swayc_active_output();

		if (config->mouse_warping && old_output != new_output) {
			swayc_t *focused = get_focused_view(ws);
			if (focused && focused->type == C_VIEW) {
				center_pointer_on(focused);
			}
		}
	} else {
		if (strcasecmp(argv[1], "output") == 0) {
			if ((error = checkarg(argc, "workspace", EXPECTED_EQUAL_TO, 3))) {
				return error;
			}
			struct workspace_output *wso = calloc(1, sizeof(struct workspace_output));
			if (!wso) {
				return cmd_results_new(CMD_FAILURE, "workspace output",
						"Unable to allocate workspace output");
			}
			wso->workspace = strdup(argv[0]);
			wso->output = strdup(argv[2]);
			int i = -1;
			if ((i = list_seq_find(config->workspace_outputs, workspace_output_cmp_workspace, wso)) != -1) {
				struct workspace_output *old = config->workspace_outputs->items[i];
				free(old); // workspaces can only be assigned to a single output
				list_del(config->workspace_outputs, i);
			}
			sway_log(L_DEBUG, "Assigning workspace %s to output %s", argv[0], argv[2]);
			list_add(config->workspace_outputs, wso);
			if (!config->reading) {
				// TODO: Move workspace to output. (don't do so when reloading)
			}
		}
	}
	return cmd_results_new(CMD_SUCCESS, NULL, NULL);
}
