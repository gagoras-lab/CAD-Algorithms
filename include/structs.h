#ifndef _STRUCTS_H
#define _STRUCTS_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <tcl8.6/tcl.h>
#include <cudd.h>

#define LINE_MAX 1024                   // Max length of a command
#define LEN_MAX 30                      // Max length of a name of any struct
#define SHRT_MAX 32767                  // Max unsigned short value
#define MAX_SUBSTRINGS 100              // Max number of substrings in a line
typedef enum {NOT_SET_IO, PRIMARY_INPUT, PRIMARY_OUTPUT} io_type; // Type of each IO node
typedef enum {CELL_INPUT, CELL_OUTPUT} cell_pin_type;             // Type of gatepin on the cell

typedef struct
  {
    char name[LEN_MAX];                 
    DdNode *node;                       // Node of the BDD tree
    double probability;                 // Static probability of the current DDnode
  } ProbabilityNode;

typedef struct
  {
    DdManager* manager;                 // Manager of the BDD tree
    ProbabilityNode* top;               // Top BDD Node of the tree
  } BDDTable;

typedef struct
  {
    char name[LEN_MAX];                 // Instance name
    char owner[LEN_MAX];                // Name of the owner component (gate)
    char function[LEN_MAX];             // Boolean function computed in this component
    short level;                        // Level of the gatepin in the hierarchy (primary inputs in level 0)
    cell_pin_type pin_type;             // Parameter indicating if the gatepins is an input or output
    bool ready;                         // Parameter used for levelization purposes
    BDDTable BDDTable;                  // BDD table-tree of the gatepin
    double static_probability;          // Value of the static probability of the state logic-1 or logic-0;
  } gatepin;

typedef struct
  {
    char name[LEN_MAX];                 // Instance name
    char cell_type[LEN_MAX];            // Library cell name
    char cell_timing_type[LEN_MAX];     // Combinational or Sequential
    float dimensions[2];                // {Width, Height}
    gatepin **CCs;                      // Pointer to a dynamic array of pointers of the connected gatepins
    int CC_count;                       // Size of the 'connected_pins' dynamic array
    gatepin **cell_output;                  // Output of the gate to a node
    unsigned short out_count;           // Number of output pins of the component
    short level;                        // Level of the component in the circuit
    bool ready;                         // Parameter used for levelization purposes
    gatepin **cell_input;               // Input gatepins of the component
    unsigned short in_count;            // Number of input pins of the component
  } component;

typedef struct
  {
    char name[LEN_MAX];                 // Instance name
    gatepin **CCs;                      // Pointer to a dynamic arraayof pointers pointers to the gates, which the gatepin is connected
    int CC_count;                       // Size of the dynamic array of gatepins
    io_type io_type;                    // Type of I/O
  } IO_node;

typedef struct
  {
    IO_node **instance;                 // Pointer to a dynamic array of IOs
    int size;                           // Size of the dynamic array of IOs
    int pin_count;                      // Amount of gatepins found connected with the I/O nodes
  } interface;

typedef struct
  {
    component **instance;               // Pointer to a dynamic array of gate instances
    int size;                           // Size of the dynamic array of gate instances
    int pin_count;                      // Amount of gatepins found connected with the circuit components
  } gates;

#endif