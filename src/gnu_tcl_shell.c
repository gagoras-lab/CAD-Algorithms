#include <dirent.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "structs.h"
#include "functions.h"

// Custom TCL interpreter used //
static Tcl_Interp *interp;

// Custom tab completion function for Tcl commands and file names //
static char** tcl_completion(const char* text, int start, int end)
  {
    char** matches = NULL;
    int status;

    // Readline Completion Variables //
    rl_completion_suppress_append = 1;
    rl_completion_append_character = '\0';

    // Use Tcl's info commands to get a list of available commands //
    status = Tcl_Eval(interp, "info commands");
    if (status == TCL_OK && start == 0)
      {
        // Split the result into a Tcl list //
        const char *commands = Tcl_GetStringResult(interp);
        int count;
        const char **cmdList;
        status = Tcl_SplitList(interp, commands, &count, &cmdList);
        
        if (status == TCL_OK)
          {
            // Iterate through the list and find matches for text //
            int matchCount = 0;
            int i;

            for (i = 0; i < count; i++)
              {
                const char *cmdName = cmdList[i];

                if (strncmp(cmdName, text, end - start) == 0)
                  {
                    // Allocate memory for a new match //
                    matches = (char**)realloc(matches, (matchCount + 2) * sizeof(char*));
                    matches[matchCount] = strdup(cmdName);
                    matches[matchCount + 1] = NULL;
                    matchCount++;
                  }
              }

            // Free the memory in which the array of available commands is stored //
            Tcl_Free((char*)cmdList);

            // Matches found //
            if(matches != NULL)
              {
                // In case of one and only one match, we return that //
                if (matchCount == 1)
                  {
                    return matches;
                  }
                // Multiple matches, print them out //
                else
                  {
                    // Print all the possible matches //
                    i = 0;
                    printf("\n");
                    while(*(matches + i) != NULL)
                    {
                        printf("%s ",*(matches + i));
                        i++;
                    }
                    printf("\n");

                    // Restore the original text if there are multiple matches //
                    rl_on_new_line();
                    rl_replace_line(text, 0);
                    rl_point = rl_end;
                    rl_redisplay();
                  }
                
                // Free matches if no matches or single match //
                i = 0;
                while(*(matches + i) != NULL)
                  {
                    free(*(matches + i));
                    i++;
                  }
                free(matches);
                matches = NULL;
              }
            
            return NULL;
          }
        else
          {
            return NULL;
          }   
      }
    // In case of start != 0, we keep the Readline's default
    // completer as we work with filename completion
    return NULL;
    }

// Main function //
int main()
  {
    char* input = NULL; // readline input //
    char* expanded_input = NULL; // readline input with tab completion //
    char command[LINE_MAX]; // current command //
    int expansion_result = 0; // 0 = no expansion, 2 = no execute, else expansion done //
    int status = 0; // Check value of TCL command execution //

    // Initialize history functions //
    using_history();

    // Initialize the Tcl interpreter //
    interp = Tcl_CreateInterp();

    // Register custom Tcl commands //
    Tcl_CreateObjCommand(interp, "ls", LsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "less", LessCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "quit", QuitCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "history", HistoryCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "rm", RmCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "read_design", ReadDesignCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "list_components", ListComponentsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "list_IOs", ListIOsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_component_function", ReportCompFuncCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_component_type", ReportCompTypeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "list_component_CCs", ListComponentCCsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "list_component_gatepins", ListComponentGPsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "list_IO_CCs", ListIOCCsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_library_cell_BDD", RepLibCellBDDCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_component_BDD", RepCompBDDCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_gatepin_BDD", RepGatepinBDDCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "compute_expression_BDD", CompExpBDDCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_gatepin_level", RepGPLevelCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_gatepins_levelized", RepGPsLevelizedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_level_gatepins", RepLevelGPsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_component_level", RepComponentLevelCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_components_levelized", RepComponentsLevelizedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "report_level_components", RepLevelComponentsCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "set_static_probability", SetStaticProbCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "list_static_probabilities", ListStaticProbCmd, NULL, NULL);

    // Set up GNU Readline //
    rl_completion_entry_function = NULL; // Initialized to NULL, the default filename completer //
    rl_attempted_completion_function = (rl_completion_func_t*) tcl_completion; // Completion from tcl_completion function to readline //
    rl_completion_append_character = '\0'; // After completion, no extra character is added //

    while ((input = readline("tcl> ")) != NULL)
      {
        if (input[0] != '\0')
          {
            // Add the input to the Readline history
            expansion_result = history_expand(input, &expanded_input);
            if ((expansion_result == 0) || // no expansion //
                (expansion_result == 2)) // do not execute //
              {
                // Add the input to the Readline history //
                add_history(input);
                strcpy(command, input); // store command //
              }
            else
              {
                // Add the expanded input to the Readline history //
                add_history(expanded_input);
                strcpy(command, expanded_input); // store command //
              }

            free(input);
            input = NULL;
            free(expanded_input);
            expanded_input = NULL;
            
            // Execute the Tcl command
            status = Tcl_Eval(interp, command);
            if (status != TCL_OK)
              {
                fprintf(stderr, "Tcl error: %s\n", Tcl_GetStringResult(interp));
              }
          }
        // In case of "quit" command, we have to first clear the history entries //
        if(!strcmp("quit", command) && status == TCL_OK)
          {
            break;
          }
      }

    // Deleting the interpreter
    Tcl_DeleteInterp(interp);
    Tcl_Finalize();
    return 0;
  }