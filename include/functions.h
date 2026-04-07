#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

extern void list_components();                                                                           // Print all components
extern void list_IOs();                                                                                  // Print all IOs
extern void report_component_func(char component_name[]);                                                // Report the boolean function of a specific component
extern void report_component_type(char component_name[]);                                                // Report the type (Sequential or Combinational) of a specific component
extern void report_component_ccs(char component_name[]);                                                 // Report the connected components of a specific component
extern void report_component_gps(char component_name[]);                                                 // Report the connected gatepins of a specific component
extern void report_all_component_gps();                                                                  // Report the connected gatepins of all components
extern void report_io_ccs(char io_name[]);                                                               // Report the connected components of an IO node
extern int ReadDesign(const char filename[]);                                                            // Main function of reading a circuit design from a file
extern void clear_circuit_parser();                                                                      // Free all dynamic memory allocated by the circuit parser
extern void report_lib_cell_BDD(char component_name[]);                                                  // Report the library cell's BDD
extern void report_component_BDD(char component_name[]);                                                 // Report the component's BDD
extern void compute_expression_BDD(char boolean_infix[]);                                                // Compute a boolean expression's BDD
extern int report_gp_level(char component_name[], char gp_name[]);                                       // Function to report a gatepin's level
extern void report_level_gps(int searched_level);                                                        // Function to report a level's gatepins
extern void report_gps_levelized();                                                                      // Function to report the circuit's gatepins, sorted by level
extern int report_component_level(char cc_name[]);                                                       // Function to report a component's level
extern void report_level_components(int searched_level);                                                 // Function to report a level's components
extern void report_components_levelized();                                                               // Function to report the circuit's components, sorted by level
extern void report_gatepin_BDD(char component_name[], char gatepin_name[]);                              // Report the gatepin's BDD
extern void set_probability_all(double probability_value);                                               // Function to set the given static probability to all startpoint gatepins of the circuit
extern void set_probability_chosen(char owner_name[], char gatepin_name[], double probability_value);    // Function to set the given static probability to a given gatepin of the circuit
extern void list_probabilities_all();                                                                    // Function to list static probabilities of all gatepins of the circuit
extern void list_probabilities_chosen(char owner_name[], char gatepin_name[]);                           // Function to list static probability of given gatepin of the circuit

// Function prototype for custom Tcl commands //
extern int LsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int LessCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int QuitCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int HistoryCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RmCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ReadDesignCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ListComponentsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ListIOsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ReportCompFuncCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ReportCompTypeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ListComponentCCsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ListComponentGPsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ListIOCCsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepLibCellBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepCompBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int CompExpBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepGPLevelCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepGPsLevelizedCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepLevelGPsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepComponentLevelCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepComponentsLevelizedCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepLevelComponentsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int RepGatepinBDDCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int SetStaticProbCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
extern int ListStaticProbCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);

#endif