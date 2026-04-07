#include <dirent.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "structs.h"
#include "functions.h"

// Custom Tcl command for 'ls' //
int LsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1,2]  //
    // Examples:    ls                                          //
    //              ls <directory>                              //
    if (objc > 3)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "directory");
        return TCL_ERROR;
      }

    // Flag for access to hidden files/directories
    // '\0' = no access
    //  '1' = In directories, do not ignore file names that start with ‘.’
    char access = '\0';
    const char *directory;

    // Only one argument, we're already in the desired directory //
    if(objc == 1)
      {
        directory = ".";
      }
    // Two arguments //
    else if(objc == 2)
      {
        const char* option = Tcl_GetString(objv[1]);
        // Second argument is option, so we're already in the desired directory
        // Option handling 
        if(option[0] == '-')
          {
            directory = ".";
            if(option[1] == 'a') // Access to every file/directory //
              {
                access = '1';
              }
            else if(option[1] == 'l') // No access to hidden file/directory //
              {
                access = '\0';
              }
            else
              {
                Tcl_SetResult(interp, "Failed to execute 'ls' command", TCL_STATIC);
                return TCL_ERROR;
              }
          }
        else 
          {
            directory = option;
          }
      }
    // Three arguments //
    else
      {
        // Option handling //
        const char* option = Tcl_GetString(objv[1]);
        if(!strcmp(option, "-a")) // Access to every file/directory //
          {
            access = '1';
          }
        else if(!strcmp(option, "-l")) // No access to hidden file/directory //
          {
            access = '\0';
          }
        else
          {
            Tcl_SetResult(interp, "Failed to execute 'ls' command", TCL_STATIC);
            return TCL_ERROR;
          }
        // Directory name stands in third argument
        directory = Tcl_GetString(objv[2]);
      }
    
    DIR *directory_pointer;
    struct dirent *entry;
    directory_pointer = opendir(directory);

    if(directory_pointer == NULL)
      {
        Tcl_SetResult(interp, "Failed to execute 'ls' command", TCL_STATIC);
        return TCL_ERROR;
      }
    
    while((entry = readdir(directory_pointer)))
      {
        if(access == '1' || (access == '\0' && entry->d_name[0] != '.')) // Handling of hidden files/directories //
          printf("%s\n",entry->d_name);
      }

    return TCL_OK;
  }

// Custom Tcl command for 'less' //
int LessCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]    //
    // Example: less <filename>                                 //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
      }

    const char* filename = Tcl_GetString(objv[1]);
    char command[1024];
    snprintf(command, sizeof(command), "less %s", filename);

    // Execute the 'less' command //
    int status = system(command);

    if (status != 0)
      {
        Tcl_SetResult(interp, "Failed to execute 'less' command", TCL_STATIC);
        return TCL_ERROR;
      }

    return TCL_OK;
  }

// Custom Tcl command for 'quit' //
int QuitCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1]    //
    // Example: quit                                            //
    if (objc != 1)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
      }

    // Exit the Tcl shell //
    rl_clear_history();
    clear_circuit_parser();
    printf("Quitting, goodbye!\n");
    return TCL_OK;
  }

// Custom Tcl command for 'history' //
int HistoryCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1]    //
    // Example: history                                         //
    if (objc != 1)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
      }

    // Get the command history from GNU Readline //
    HIST_ENTRY **history = history_list();

    // Display the command history //
    for (int i = 0; history[i] != NULL; i++)
      {
        printf("%d: %s\n", (i + history_base), (*(history + i))->line);
      }

    return TCL_OK;
  }

// Custom Tcl command for removing files or directories //
int RmCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2, ...)   //
    // Examples: rm <options> ... <filename>...                     //
    if (objc < 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "fileOrDir1 ?fileOrDir2 ...?");
        return TCL_ERROR;
      }

    // Iterate through the arguments and remove each file or directory //
    for (int i = 1; i < objc; i++)
      {
        const char* fileOrDir = Tcl_GetString(objv[i]);

        // Use the unlink function to remove a file or rmdir to remove a directory //
        if (access(fileOrDir, F_OK) == 0)
          {
            if (remove(fileOrDir) != 0)
              {
                Tcl_SetResult(interp, "Failed to remove the specified file or directory", TCL_STATIC);
                return TCL_ERROR;
              }
          }
        else
          {
            Tcl_SetResult(interp, "Specified file or directory does not exist", TCL_STATIC);
            return TCL_ERROR;
          }
      }

    return TCL_OK;
  }

// Custom Tcl command for reading a design from a file //
int ReadDesignCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: read_design <filename>                        //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
      }

    const char* DesignFile = Tcl_GetString(objv[1]);
    if (access(DesignFile, F_OK) == 0)
      {
        if (ReadDesign(DesignFile) != 0)
          {
            Tcl_SetResult(interp, "Failed to read the specified file", TCL_STATIC);
            return TCL_ERROR;
          }
      }
    else
      {
        Tcl_SetResult(interp, "Specified file does not exist", TCL_STATIC);
        return TCL_ERROR;
      }

    return TCL_OK;
  }

// Custom Tcl command for listing all components of the design //
int ListComponentsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1]   //
    // Examples: list_components                               //
    if (objc != 1)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
      }

    list_components();
    return TCL_OK;
  }

// Custom Tcl command for listing all I/O's of the design //
int ListIOsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1]   //
    // Examples: list_IOs                                      //
    if (objc != 1)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
      }

    list_IOs();
    return TCL_OK;
  }

// Custom Tcl command for reporting a component's boolean function //
int ReportCompFuncCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_component_func <component_name>        //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "component_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    report_component_func(component_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting a component's cell timing type //
int ReportCompTypeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_component_type <component_name>        //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "component_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    report_component_type(component_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting a component's connected CCs //
int ListComponentCCsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: list_component_CCs <component_name>        //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "component_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    report_component_ccs(component_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting a component's gatepins //
int ListComponentGPsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]                //
    // Examples: list_component_gatepins <component_name> | -allcomponents  //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "component_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    if(!strcmp(component_name, "-allcomponents"))
      {
        report_all_component_gps();
      }
    else
      {
        report_component_gps(component_name);
      }

    return TCL_OK;
  }

// Custom Tcl command for reporting an I/O's connected CCs //
int ListIOCCsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_IO_CCs <io_name>        //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "io_name");
        return TCL_ERROR;
      }

    char* io_name = Tcl_GetString(objv[1]);
    report_io_ccs(io_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting a library cell's BDD //
int RepLibCellBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_library_cell_BDD <component_name>      //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "lib_cell_name");
        return TCL_ERROR;
      }

    char* libcell_name = Tcl_GetString(objv[1]);
    report_lib_cell_BDD(libcell_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting a components's BDD //
int RepCompBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_component_BDD <component_name>         //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "component_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    report_component_BDD(component_name);

    return TCL_OK;
  }

// Custom Tcl command for computing a boolean expression's BDD //
int CompExpBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: compute_expression_BDD <boolean_expression>   //
    if (objc < 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "boolean_expression");
        return TCL_ERROR;
      }
    
    char *bool_expression = calloc(LEN_MAX, sizeof(char));
    strcpy(bool_expression, Tcl_GetString(objv[1]));

    // Handling spaces in the boolean expression //
    for(int i = 2; i < objc; i++)
      {
        strcat(bool_expression, Tcl_GetString(objv[i]));
      }
    
    compute_expression_BDD(bool_expression);

    free(bool_expression);
    bool_expression = NULL;

    return TCL_OK;
  }

// Custom Tcl command for reporting a gatepin's level //
int RepGPLevelCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_gatepin_level <gatepin_name>           //
    if (objc != 3)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "gatepin_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    char* gp_name = Tcl_GetString(objv[2]);
    report_gp_level(component_name, gp_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting the circuit's gatepins, sorted by level //
int RepGPsLevelizedCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1]   //
    // Examples: report_gatepins_levelized                     //
    if (objc != 1)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
      }
    
    report_gps_levelized();

    return TCL_OK;
  }

// Custom Tcl command for reporting a level's gatepins //
int RepLevelGPsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_level_gatepins <level>                 //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "level");
        return TCL_ERROR;
      }

    int level = atoi(Tcl_GetString(objv[1]));
    report_level_gps(level);

    return TCL_OK;
  }

// Custom Tcl command for reporting a component's level //
int RepComponentLevelCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_gatepin_level <component_name>         //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "component_name");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    report_component_level(component_name);

    return TCL_OK;
  }

// Custom Tcl command for reporting the circuit's gatepins, sorted by level //
int RepComponentsLevelizedCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [1]   //
    // Examples: report_components_levelized                   //
    if (objc != 1)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
      }
    
    report_components_levelized();

    return TCL_OK;
  }

// Custom Tcl command for reporting a level's gatepins //
int RepLevelComponentsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2]   //
    // Examples: report_level_components <level>               //
    if (objc != 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "level");
        return TCL_ERROR;
      }

    int level = atoi(Tcl_GetString(objv[1]));
    report_level_components(level);

    return TCL_OK;
  }

// Custom Tcl command for reporting a gatepin's BDD //
int RepGatepinBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [3]          //
    // Examples: report_component_BDD <component_name> <gatepin_name> //
    if (objc != 3)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "<component_name> <gatepin_name>");
        return TCL_ERROR;
      }

    char* component_name = Tcl_GetString(objv[1]);
    char* gatepin_name = Tcl_GetString(objv[2]);
    report_gatepin_BDD(component_name, gatepin_name);

    return TCL_OK;
  }

// Custom Tcl command for setting static probability to the gatepins of the circuit //
int SetStaticProbCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [4, 5]                                                  //
    // Examples: set_static_probability -value <probability value> -gatepins {gatepin list} | -allstartpoints //
    if (objc < 3)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "-value <probability value> -gatepins {gatepins list} | -allstartpoints");
        return TCL_ERROR;
      }
    
    char *eptr;
    double prob_value = strtod(Tcl_GetString(objv[2]), &eptr);
    char* gatepin_choice = Tcl_GetString(objv[3]);

    if(prob_value > 1 || prob_value < 0)
      {
        printf("Not compatible value of probability. Value must be between 0 and 1!\n");
      }
    else if(!strcmp(gatepin_choice, "-allstartpoints"))
      {
        set_probability_all(prob_value);
      }
    else if (!strcmp(gatepin_choice, "-gatepins"))
      {
        for(int i = 4; i < objc; i = i + 2)
          {
            char *gatepin_owner = Tcl_GetString(objv[i]);
            char *gatepin_name = Tcl_GetString(objv[i+1]);
            
            set_probability_chosen(gatepin_owner, gatepin_name, prob_value);
          }
      }

    return TCL_OK;
  }

// Custom Tcl command for listing static probability of the gatepins of the circuit //
int ListStaticProbCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
  {
    // Check for correct amount of arguments of command: [2, 3]                     //
    // Examples: list_static_probabilities -gatepins {gatepins list} | -allgatepins //
    if (objc < 2)
      {
        Tcl_WrongNumArgs(interp, 1, objv, "-gatepins {gatepins list} | -allgatepins");
        return TCL_ERROR;
      }
    
    char* gatepin_choice = Tcl_GetString(objv[1]);

    if(!strcmp(gatepin_choice, "-allgatepins"))
      {
        list_probabilities_all();
      }
    else if (!strcmp(gatepin_choice, "-gatepins"))
      {
        for(int i = 2; i < objc; i = i + 2)
          {
            char *gatepin_owner = Tcl_GetString(objv[i]);
            char *gatepin_name = Tcl_GetString(objv[i+1]);
            
            list_probabilities_chosen(gatepin_owner, gatepin_name);
          }
      }

    return TCL_OK;
  }