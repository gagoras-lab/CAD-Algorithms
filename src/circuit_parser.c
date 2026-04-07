#include "structs.h"
#include "functions.h"
#include <sys/types.h>
#include <ctype.h>
#include <assert.h>

static interface *circuit_io;
static gates     *circuit_cells;
static unsigned short MaxCircuitLevel;

///////////////////////////////////////
//////     UTILITY FUNCTIONS    ///////
///////////////////////////////////////

unsigned int hash(char *str, int capacity)
  { // Hash function

    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return (hash % capacity);
  }

void clean_str(char str[])
  { // General function for cleaning strings

    memset(str, '\0', LEN_MAX);
  }

char** str_split(char* text, const char pre_delim)
  { // Split a string into substrings with the indication being a specific character

    char** result = 0;
    size_t count = 0;
    char* temp = text;
    char* last_comma = 0;
    char delim[2];
    delim[0] = pre_delim;
    delim[1] = 0;

    // Count how many elements will be extracted
    while (*temp)
      {
        if (pre_delim == *temp)
        {
            count++;
            last_comma = temp;
        }
        temp++;
      }

    // Add space for trailing token
    count += last_comma < (text + strlen(text) - 1);

    // Add space for terminating null string so caller
    // knows where the list of returned strings ends
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
      {
        size_t idx  = 0;
        char* token = strtok(text, delim);

        while (token)
          {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
          }
        assert(idx == count - 1);
        *(result + idx) = 0;
      }

    return result;
  }

void clear_tokens(char **tokens)
  { // Helping functiion to clear a dynamic array of pointers

    int i;
    
    for(i = 0; *(tokens + i); i++)
      {
        free(*(tokens+i));
        *(tokens+i) = NULL;
      }
    free(tokens);
  }

///////////////////////////////////////
////////     CIRCUIT PARSER    ////////
///////////////////////////////////////

component *component_creator(char name[], char cell_type[], char cell_timing_type[], double width, double height)
  { // Initialization of a component

    component *component_instance;

    component_instance = (component *) malloc(sizeof(component));
    if (component_instance == NULL)
      {
          printf("Error: component_creator malloc failed.\n");
          exit(EXIT_FAILURE);
      }

    // Initialization of the new instance of component
    strcpy(component_instance->name, name);
    strcpy(component_instance->cell_type, cell_type);
    strcpy(component_instance->cell_timing_type, cell_timing_type);
    component_instance->dimensions[0] = width;
    component_instance->dimensions[1] = height;

    component_instance->CCs = NULL;
    component_instance->CC_count = 0;

    component_instance->cell_output = NULL;
    component_instance->out_count = 0;

    component_instance->ready = false;
    component_instance->level = -1;

    component_instance->cell_input = NULL;
    component_instance->in_count = 0;

    return (component_instance);
  }

IO_node *IO_creator(char name[])
  { // Initialization of a Top-Level I/O

    IO_node *IO_instance;

    IO_instance = (IO_node *) malloc(sizeof(IO_node));
    if (IO_instance == NULL)
      {
          printf("Error: IO_creator malloc failed.\n");
          exit(EXIT_FAILURE);
      }

    // Initialization of the new instance of component
    strcpy(IO_instance->name, name);
    IO_instance->io_type = NOT_SET_IO;

    IO_instance->CCs = NULL;
    IO_instance->CC_count = 0;

    return (IO_instance);
  }

gatepin *gatepin_creator(char name[], char owner_gate_name[])
  { // Initialization of a gatepin

    gatepin *new_instance;

    new_instance = (gatepin *) malloc(sizeof(gatepin));

    if (new_instance == NULL)
      {
          printf("Error: gatepin_creator malloc failed.\n");
          exit(EXIT_FAILURE);
      }

    // Initialization of the new instance of node
    strcpy(new_instance->name, name);
    strcpy(new_instance->owner, owner_gate_name);
    memset(new_instance->function, '\0', LEN_MAX);
    new_instance->level = -1;
    new_instance->pin_type = CELL_INPUT;
    new_instance->ready = false;
    new_instance->BDDTable.manager = NULL;
    new_instance->BDDTable.top = NULL;
    new_instance->static_probability = 0.5;

    return (new_instance);
  }

void add_gatepin(component *owner_gate, gatepin *new_gp)
  { // Addition of a gatepin to its owner

    gatepin **instance_check;
    int pin_count = owner_gate->CC_count;

    instance_check = (gatepin **) realloc(owner_gate->CCs, (pin_count + 1) * sizeof(gatepin*));

    if (instance_check == NULL)
      {
          printf("Error: add_gatepin realloc failed.\n");
          exit(EXIT_FAILURE);
      }

    owner_gate->CCs = instance_check;
    owner_gate->CCs[pin_count] = new_gp;
    owner_gate->CC_count++;  
  }

void add_output_pin(component *owner_gate, gatepin *new_out_gp, char boolean_function[])
  { // Addition of an output gatepin to its owner

    gatepin **instance_check;
    int pin_count = owner_gate->out_count;

    instance_check = (gatepin **) realloc(owner_gate->cell_output, (pin_count + 1) * sizeof(gatepin*));

    if (instance_check == NULL)
      {
          printf("Error: add_gatepin realloc failed.\n");
          exit(EXIT_FAILURE);
      }

    owner_gate->cell_output = instance_check;
    owner_gate->cell_output[pin_count] = new_out_gp;
    strcpy(new_out_gp->function, boolean_function);
    new_out_gp->pin_type = CELL_OUTPUT;
    owner_gate->out_count++;  
  }

void add_input_pin(component *owner_gate, gatepin *new_in_gp)
  { // Addition of an input gatepin to its owner

    gatepin **instance_check;
    int pin_count = owner_gate->in_count;

    instance_check = (gatepin **) realloc(owner_gate->cell_input, (pin_count + 1) * sizeof(gatepin*));

    if (instance_check == NULL)
      {
          printf("Error: add_gatepin realloc failed.\n");
          exit(EXIT_FAILURE);
      }

    owner_gate->cell_input = instance_check;
    owner_gate->cell_input[pin_count] = new_in_gp;
    owner_gate->in_count++;  
  }

void add_cc(gates *circuit_cells, component *component_instance)
  { // Addition of a new component to the dynamic array of saved components
    
    int index = hash(component_instance->name, circuit_cells->size);

    while (circuit_cells->instance[index] != NULL)
      {
        index = (index + 1) % circuit_cells->size;
      }
    
    circuit_cells->instance[index] = component_instance;
  }

void add_io(interface *circuit_io, IO_node *io_instance)
  { // Addition of a new component to the dynamic array of saved components

    int index = hash(io_instance->name, circuit_io->size);

    while (circuit_io->instance[index] != NULL)
      {
        index = (index + 1) % circuit_io->size;
      }
    
    circuit_io->instance[index] = io_instance;  
  }

void gatepin_to_cc(component *connected_gate, gatepin *node)
  { // Node is an input of the gate, so they must be connected

    int new_size = connected_gate->CC_count + 1;
    connected_gate->CCs = realloc(connected_gate->CCs, new_size * sizeof(gatepin*));
    if (connected_gate->CCs  == NULL)
      {
        printf("Error: gatepin_to_cc realloc failed.\n");
        exit(EXIT_FAILURE);
      }

    connected_gate->CCs[connected_gate->CC_count] = node;
    connected_gate->CC_count++;
  }

void IO_to_cc(IO_node *connected_IO, gatepin *node)
  { // Node is an input of the gate, so they must be connected.

    int new_size = connected_IO->CC_count + 1;
    connected_IO->CCs = realloc(connected_IO->CCs, new_size * sizeof(gatepin*));
    if (connected_IO->CCs == NULL)
    {
        printf("Error: IO_to_cc realloc failed.\n");
        exit(EXIT_FAILURE);
    }

    connected_IO->CCs[connected_IO->CC_count] = node;
    connected_IO->CC_count++;
  }

component *find_libcell(gates *circuit_cells, char libcell_name[])
  { // Find the component with the given libcell name in the equivalent dynamic array

    int index;

    for(index = 0; index < circuit_cells->size; index++)
      {
        if(!strcmp(circuit_cells->instance[index]->cell_type, libcell_name))
          {
            return circuit_cells->instance[index];
          }
      }
    
    return NULL;
  }

component *find_component(gates *circuit_cells, char cc_name[])
  { // Find the component with the given name in the equivalent dynamic array

    int index = hash(cc_name, circuit_cells->size);

    while (circuit_cells->instance[index] != NULL)
      {
        if(!strcmp(circuit_cells->instance[index]->name, cc_name))
          {
            return circuit_cells->instance[index]; 
          }
        index = (index + 1) % circuit_cells->size;
      }
    
    return NULL;
  }

IO_node *find_io(interface *circuit_io, char io_name[])
  { // Find the IO pin with the given name in the equivalent dynamic array
    
    int index = hash(io_name, circuit_io->size);

    while (circuit_io->instance[index] != NULL)
      {
        if(!strcmp(circuit_io->instance[index]->name, io_name))
          {
            return circuit_io->instance[index]; 
          }
        index = (index + 1) % circuit_io->size;
      }

    return NULL;
  }

gatepin *find_gatepin(component *gate, char gatepin_name[])
  { // Find the gatepin with the given name in the given component (gate)

    int i;

    for (i = 0; i < gate->CC_count; i++)
      {
        if(!strcmp(gate->CCs[i]->name, gatepin_name) && !strcmp(gate->CCs[i]->owner, gate->name))
          {
            return gate->CCs[i];
          }
      }

    return NULL;
  }

void update_input_gatepins()
  { // Add the according input gatepins to each component

    int i, j;
    gatepin *gp_pointer, *curr_gp;
    component *cc_pointer;

    for(i = 0; i < circuit_io->size; i++)
      {
        for(j = 0; j < circuit_io->instance[i]->CC_count; j++)
          {
            curr_gp = circuit_io->instance[i]->CCs[j];
            cc_pointer = find_component(circuit_cells, curr_gp->owner);
            gp_pointer = find_gatepin(cc_pointer, curr_gp->name);
            if(gp_pointer == NULL)
              {
                add_input_pin(cc_pointer, curr_gp);
              }
          }
      }
    
    for(i = 0; i < circuit_cells->size; i++)
      {
        for(j = 0; j < circuit_cells->instance[i]->CC_count; j++)
          {
            curr_gp = circuit_cells->instance[i]->CCs[j];
            cc_pointer = find_component(circuit_cells, curr_gp->owner);
            gp_pointer = find_gatepin(cc_pointer, curr_gp->name);
            if(gp_pointer == NULL && strcmp(circuit_cells->instance[i]->name, curr_gp->owner) != 0)
              { // Making sure we don't stand in the circuit cell we're refering to
                add_input_pin(cc_pointer, curr_gp);
              }
          }
      }
  }

void update_io_types()
  { // Function to update if the I/O nodes are primary inputs or primary outputs

    int i = 0, j = 0;
    gatepin *io_gp_pointer, *cell_gp_pointer;
    IO_node *io_pointer;
    component *cell_pointer;

    for(i = 0; i < circuit_io->size; i++)
      {
        io_pointer = circuit_io->instance[i];
        if(io_pointer->io_type == NOT_SET_IO)
          { // I/O hasn't been set yet
            for(j = 0; j < io_pointer->CC_count; j++)
              {
                io_gp_pointer = circuit_io->instance[i]->CCs[j];
                cell_pointer = find_component(circuit_cells, io_gp_pointer->owner);
                cell_gp_pointer = find_gatepin(cell_pointer, io_gp_pointer->name);
                if(cell_gp_pointer == NULL)
                  { // If the gatepin name isn't found in the CCs of the owner, it is a primary input
                    io_pointer->io_type = PRIMARY_INPUT;
                    io_gp_pointer->level = 0; // Primary inputs are on level 1
                  }
                else
                  { // Primary output case
                    io_pointer->io_type = PRIMARY_OUTPUT;
                    cell_gp_pointer->level = -2; // Temporary value of the primary output's level
                  }
              }
          }
        // For Primary input, set also level of according gatepins to 1
        else if(io_pointer->io_type == PRIMARY_INPUT)
          {
            for(j = 0; j < io_pointer->CC_count; j++)
              {
                io_pointer->CCs[j]->level = 1;
              }
          }
        // For Primary output, set also level of according gatepins to temporary value "USHRT_MAX - 1"
        else if(io_pointer->io_type == PRIMARY_OUTPUT)
          {
            for(j = 0; j < io_pointer->CC_count; j++)
              {
                io_pointer->CCs[j]->level = -2;
              }
          }
      }
  }

void calculate_capacity(FILE *parser_file, int *comp_ht_capacity, int *io_ht_capacity)
  { // First parse of the circuit file, to calculate the capacity of both the Components' and I/O's hash tables

    char *line = NULL;                              // Every line of the given file will be saved here for each iteration
    char **tokens = NULL;                           // Pointer of dynamic array holding the substrings of the line of each iteration
    size_t len = 0;                                 // Needed for readline(): max length of each line read
    ssize_t read_size = 0;                          // Needed for readline(): actual length of each line read
    char *last_CC = calloc(LEN_MAX, sizeof(char));  // Last component found
    char *curr_CC = calloc(LEN_MAX, sizeof(char));  // Current component

    while ((read_size = getline(&line, &len, parser_file)) != -1)
      { // Skipping all comments and unneeded information
        if(!strcmp(line,"# Top-Level I/O CCs:\n"))
          break;
      }
    while ((read_size = getline(&line, &len, parser_file)) != -1)
      {
        // Preparation of the line, before it gets split into sub-strings
        if(line[0] == '#')
          {
            continue;
          }
        else if(line[0] == 'C')
          {
            line[read_size-1] = ' ';
          }
        else if(line[0] == 'I')
          {
            line[read_size - 1] = '\0';
            line[read_size - 2] = '\0';
          }
        
        // Split the current line into strings //
        tokens = str_split(line, ' ');
        
        if(!strcmp(*(tokens), "IO:"))
          { // New I/O Case
            *io_ht_capacity = *io_ht_capacity + 1;
          }
        else if(!strcmp(*(tokens), "Component:"))
          { // New Component Case
            clean_str(curr_CC);
            strncpy(curr_CC, *(tokens + 1), strlen(*(tokens + 1)) - 1);

            // If the name of the current component is the same as the one in the last line,
            // we're on the output information of the same component, not a new entry
            if(strcmp(curr_CC, last_CC) != 0)
              {
                *comp_ht_capacity = *comp_ht_capacity + 1;
                clean_str(last_CC);
                strcpy(last_CC, *(tokens + 1));
              }
          }
        clear_tokens(tokens);      
      }
    
    rewind(parser_file);

    free(line);
    line = NULL;
    free(curr_CC);
    curr_CC = NULL;
    free(last_CC);
    last_CC = NULL;
  }

void circuit_parser(FILE *parser_file, int *comp_ht_capacity, int *io_ht_capacity)
  { // Main function that handles the circuit parser

    int i = 0;
    gatepin *gp_pointer = NULL;       // Temporary gatepin pointer
    component *cc_pointer = NULL;     // Temporary cc pointer
    char *line = NULL;                // Every line of the given file will be saved here for each iteration
    char *CC = malloc(LEN_MAX * sizeof(char));                    // CC name
    char *CC_gatepin = malloc(LEN_MAX * sizeof(char));            // Gatepin name
    char **tokens;                    // Pointer of dynamic array holding the substrings of the line of each iteration
    size_t len = 0;                   // Needed for readline(): max length of each line read
    ssize_t read_size = 0;            // Needed for readline(): actual length of each line read
    long start_of_next_line = 0;      // Temporary file pointer, pointing to the start of the next line to be read

    printf("Circuit_parser function is called.\n");

    // Intialization of the IO interface //
    circuit_io = (interface *) malloc(sizeof(interface));
    if (circuit_io == NULL)
      {
        printf("Error: Malloc for interface *circuit_io pointer failed.\n");
        exit(EXIT_FAILURE);
      }
    circuit_io->size = *io_ht_capacity;
    circuit_io->pin_count = 0;
    circuit_io->instance = (IO_node **) calloc(circuit_io->size, sizeof(IO_node*));

    // Initialization of the Component Dynamic array //
    circuit_cells = (gates *) malloc(sizeof(gates));
    if (circuit_cells == NULL)
      {
        printf("Error: Malloc for gates *circuit_cells pointer failed.\n");
        exit(EXIT_FAILURE);
      }
    circuit_cells->size = *comp_ht_capacity;
    circuit_cells->pin_count = 0;
    circuit_cells->instance = (component**) calloc(circuit_cells->size, sizeof(component*));
    
    // SAVE COMPONENTS //
    while ((read_size = getline(&line, &len, parser_file)) != -1)
      {
        if(!strcmp(line,"# Components CCs:\n"))
          break;
      }

    while ((read_size = getline(&line, &len, parser_file)) != -1)
      {
        line[read_size-1] = ' ';
        // Split the current line into strings //
        tokens = str_split(line, ' ');

        // Structure of Component CCs lines are:
        // Component: <NAME> Cell_Type: <CT> Cell_Timing_Type: <CTT> Width: <WDTH> Height: <HGHT> CCs: <CCNAME,GATEPIN>
        component *component_instance = component_creator(*(tokens + 1), *(tokens + 3), *(tokens + 5), strtod(*(tokens + 7), NULL), strtod(*(tokens + 9), NULL));
        add_cc(circuit_cells, component_instance);

        for (i = 11; *(tokens + i) && i < read_size; i = i + 2)
          {
            clean_str(CC);
            clean_str(CC_gatepin);

            strcpy(CC, *(tokens + i));
            strncpy(CC_gatepin, *(tokens + i + 1) + 2, strlen(*(tokens + i + 1)) - 3);

            cc_pointer = find_component(circuit_cells, CC);
            gp_pointer = (cc_pointer != NULL) ? find_gatepin(cc_pointer, CC_gatepin) : NULL;
            
            if(gp_pointer == NULL)  // Gatepin doesn't exist //
              {
                gp_pointer = gatepin_creator(CC_gatepin, CC);
                gatepin_to_cc(component_instance, gp_pointer);
                circuit_cells->pin_count++;
              }
            else
              {
                gatepin_to_cc(component_instance, gp_pointer);
              }
          }
        clear_tokens(tokens);

        // Save the output pins and boolean functions of current component
        while (1)
          {
            start_of_next_line = ftell(parser_file);

            // Read a second line, as we still stand in the same component //
            (read_size = getline(&line, &len, parser_file));
            if(read_size == -1)
              {
                break;
              }

            // Split the current line into strings //
            tokens = str_split(line, ' ');

            // Structure of current line is:
            // Component: <NAME> Output Pin: <PinNAME> Boolean Function: <BF_Expression>
            clean_str(CC);
            strncpy(CC, *(tokens + 1), strlen(*(tokens + 1)) - 1);

            if(strcmp(CC, component_instance->name) != 0)
              { // No more output pins left for the last component 
                clear_tokens(tokens);
                fseek(parser_file, start_of_next_line, SEEK_SET);
                break;
              }
            
            // Store the Boolean Function Expression //
            clean_str(CC);
            if(feof(parser_file))
              { // Handle end of file case, where '\n' character is missing
                strncpy(CC, *(tokens + 7), strlen(*(tokens + 7)));
              }
            else
              {
                strncpy(CC, *(tokens + 7), strlen(*(tokens + 7)) - 1);
              }

            clean_str(CC_gatepin);
            strncpy(CC_gatepin, *(tokens + 4), strlen(*(tokens + 4)) - 1);
            
            // Connect the Output Pin to the Component //
            gatepin *gp_pointer = find_gatepin(component_instance, CC_gatepin);
            add_output_pin(component_instance, gp_pointer, CC);

            clear_tokens(tokens);
          }
      }
      rewind(parser_file);

      printf("Stored all Component CCs.\n");

      // Second read of the file //
      // SAVE IOs //
      while ((read_size = getline(&line, &len, parser_file)) != -1)
        {
          if(!strcmp(line,"# Top-Level I/O CCs:\n"))
            break;
        }
      while ((read_size = getline(&line, &len, parser_file)) != -1)
        {
          if(line[0] == '#')
            { // We have reached the Connected CCs again, no need to read them twice //
              break;
            }
          line[read_size - 1] = '\0';
          line[read_size - 2] = '\0';
          // Split the current line into strings //
          tokens = str_split(line, ' ');
          
          // Structure of Top-Level I/O lines are:
          // IO: <IOName> CCs: <CCNAME,GATEPIN>
          IO_node *IO_instance = IO_creator(*(tokens + 1));
          add_io(circuit_io, IO_instance);

          for (i = 3; *(tokens + i) && i < read_size; i = i + 2)
            {
              clean_str(CC);
              clean_str(CC_gatepin);
              
              strcpy(CC, *(tokens + i));
              strncpy(CC_gatepin, *(tokens + i + 1) + 2, strlen(*(tokens + i + 1)) - 3);

              cc_pointer = find_component(circuit_cells, CC);
              gp_pointer = find_gatepin(cc_pointer, CC_gatepin);

              if(gp_pointer == NULL) // Gatepin doesn't exist //
                {
                  gp_pointer = gatepin_creator(CC_gatepin, cc_pointer->name);
                  circuit_io->pin_count++;
                }
              IO_to_cc(IO_instance, gp_pointer);
              gp_pointer->level = 1;
            }
          clear_tokens(tokens);
        }

      printf("Stored all Top-Level I/O CCs.\n");
      free(CC);
      CC = NULL;
      free(CC_gatepin);
      CC_gatepin = NULL;
      free(line);
      line = NULL;
      rewind(parser_file);

      update_input_gatepins(); // Insert the input gatepins on each gate
      update_io_types(); // Update the type of each I/O (Primary Input or Primary Output)
  }

///////////////////////////////////////
/////////     LEVELIZATION    /////////
///////////////////////////////////////

void add_ReadyGate(component **ReadyGates, component *cell_pointer, short unsigned *ReadyGatesCount, int ReadyGatesCapacity)
  { // Function to add a component from the ReadyGates array
    
    int i = 0;
    
    while(ReadyGates[i] != NULL && i < ReadyGatesCapacity)
      {
        i++;
      }
    
    cell_pointer->ready = true;
    ReadyGates[i] = cell_pointer;
    *ReadyGatesCount = *ReadyGatesCount + 1;
  }

component *findfirst_ReadyGate(component **ReadyGates, int ReadyGatesCapacity)
  { // Function to find the first available component from the ReadyGates array
    
    int i = 0;
    
    while(ReadyGates[i] == NULL && i < ReadyGatesCapacity)
      {
        i++;
      }

    return ReadyGates[i];
  }

void Remove_ReadyGate(component **ReadyGates, component *cell_pointer, short unsigned *ReadyGatesCount, int ReadyGatesCapacity)
  { // Function to remove a component from the ReadyGates array
    
    int i = 0;
    
    while(i < ReadyGatesCapacity)
      {
        if(ReadyGates[i] != NULL)
          {
            if(!strcmp(ReadyGates[i]->name, cell_pointer->name))
              {
                break;
              }
          }
        i++;
      }

    ReadyGates[i] = NULL;
    *ReadyGatesCount = *ReadyGatesCount - 1;
  }

void add_ReadyNet(gatepin **ReadyNets, gatepin *gp_pointer, short unsigned *ReadyNetsCount, int ReadyNetsCapacity)
  { // Function to add a gatepin from the ReadyNets array
    
    int i = 0;
    
    while(ReadyNets[i] != NULL && i < ReadyNetsCapacity)
      {
        i++;
      }
    
    gp_pointer->ready = true;
    ReadyNets[i] = gp_pointer;
    *ReadyNetsCount = *ReadyNetsCount + 1;
  }

gatepin *findfirst_ReadyNet(gatepin **ReadyNets, int ReadyNetsCapacity)
  { // Function to find the first available gatepin from the ReadyNets array
    
    int i = 0;
    
    while(ReadyNets[i] == NULL && i < ReadyNetsCapacity)
      {
        i++;
      }

    return ReadyNets[i];
  }

void Remove_ReadyNet(gatepin **ReadyNets, gatepin *gp_pointer, short unsigned *ReadyNetsCount, int ReadyNetsCapacity)
  { // Function to remove a gatepin from the ReadyNets array
    
    int i = 0;
    
    while(i < ReadyNetsCapacity)
      {
        if(ReadyNets[i] != NULL)
          {
            if(!strcmp(ReadyNets[i]->name, gp_pointer->name) && !strcmp(ReadyNets[i]->owner, gp_pointer->owner))
              {
                break;
              }
          }
        i++;
      }

    ReadyNets[i] = NULL;
    *ReadyNetsCount = *ReadyNetsCount - 1;
  }

void levelize_circuit()
  { // Function for levelization of the gatepins found on the given circuit
    
    unsigned short i = 0, j = 0, k = 0, l = 0;  // Function counters
    unsigned short fan_in_ready_counter = 0;    // Temporary variable to hold the amount of visited input gatepins of the current component
    unsigned short readyGatesCount = 0;         // Number of gates (components) that are ready to be levelized
    unsigned short readyNetsCount = 0;          // Number of nets (gatepin connections) that are ready to be levelized
    unsigned short MaxLevel;                    // Temporary variable to hold the max level of the input gatepins of the current component
    component *cc_pointer;
    gatepin *gp_pointer, *curr_gp;
    gatepin **ReadyNets;                        // Temporary array of the nets (gatepin connections) that are ready to be levelized
    component **ReadyGates;                     // Temporary array of the gates (components) that are ready to be levelized

    int ReadyNetsCapacity  = circuit_cells->pin_count + circuit_io->pin_count;
    int ReadyGatesCapacity = circuit_cells->size;

    // Initialization of the temporary arrays
    ReadyNets = (gatepin **) calloc(ReadyNetsCapacity, sizeof(gatepin*));
    ReadyGates = (component **) calloc(ReadyGatesCapacity, sizeof(component*));
    
    MaxCircuitLevel = 0;

    // We start the levelization process from the primary inputs of the circuit
    for(i = 0; i < circuit_io->size; i++)
      {
        // For all primary inputs, we move to the gates that they are driving, updating if or if not the are ready to be visited
        if(circuit_io->instance[i]->io_type == PRIMARY_INPUT)
          {
            for(j = 0; j < circuit_io->instance[i]->CC_count; j++)
              {
                cc_pointer = find_component(circuit_cells, circuit_io->instance[i]->CCs[j]->owner);
                if(cc_pointer->ready == true)
                  { // Current component has already been added to the ready ones
                    continue;
                  }
                
                // We check among the rest of the primary inputs if they are also connected with the referenced component
                fan_in_ready_counter = 0;
                for(k = i; k < circuit_io->size; k++)
                  {
                    if(circuit_io->instance[k]->io_type == PRIMARY_INPUT)
                      {
                        for(l = 0; l < circuit_io->instance[k]->CC_count; l++)
                          {
                            if(!strcmp(circuit_io->instance[k]->CCs[l]->owner, cc_pointer->name))
                              { // Match
                                fan_in_ready_counter++;
                              }
                          }
                      }
                  }
                
                // If all fan-in nets are ready, we add the gate to those that can be visited
                if(fan_in_ready_counter == cc_pointer->in_count)
                  {
                    cc_pointer->level = 1;
                    add_ReadyGate(ReadyGates, cc_pointer, &readyGatesCount, ReadyGatesCapacity);
                  }
              }
          }
      }
    
    // Loop of all cells found after the primary inputs
    while(readyGatesCount > 0 || readyNetsCount > 0)
      {

        if(readyGatesCount > 0)
          { // Found ready gates

            cc_pointer = findfirst_ReadyGate(ReadyGates, ReadyGatesCapacity);
            
            // Set level to the current component
            MaxLevel = 0;
            for(i = 0; i < cc_pointer->in_count; i++)
              {
                gp_pointer = cc_pointer->cell_input[i];
                if(gp_pointer->level > MaxLevel)
                  {
                    MaxLevel = gp_pointer->level;
                  }
              }
            
            // The level this component gets is the maximum level of its inputs + 1
            cc_pointer->level = MaxLevel + 1;

            for(j = 0; j < cc_pointer->CC_count; j++)
              {
                gp_pointer = cc_pointer->CCs[j];
                gp_pointer->level = cc_pointer->level;

                // The gatepins that are inputs to the next gate are added to those that can be visited
                if(gp_pointer->pin_type == CELL_INPUT)
                  {
                    add_ReadyNet(ReadyNets, gp_pointer, &readyNetsCount, ReadyNetsCapacity);
                  }  
              }
            Remove_ReadyGate(ReadyGates, cc_pointer, &readyGatesCount, ReadyNetsCapacity);
          }

        if(readyNetsCount > 0)
          { // Found ready nets

            curr_gp = findfirst_ReadyNet(ReadyNets, ReadyNetsCapacity);
            cc_pointer = find_component(circuit_cells, curr_gp->owner);

            // For the owner of the current gatepin, we check how many of its inputs already have been visited
            fan_in_ready_counter = 0;
            for(i = 0; i < cc_pointer->in_count; i++)
              {
                if(cc_pointer->cell_input[i]->level != -1)
                  {
                    fan_in_ready_counter++;
                  }
              }
            
            // If all fan-in nets are ready, we add the gate to those that can be visited
            if(fan_in_ready_counter == cc_pointer->in_count)
              {
                add_ReadyGate(ReadyGates, cc_pointer, &readyGatesCount, ReadyGatesCapacity);
              }
            Remove_ReadyNet(ReadyNets, curr_gp, &readyNetsCount, ReadyNetsCapacity);
          }
      }
    
    // Update the max level of the circuit
    for(i = 0; i < circuit_cells->size; i++)
      {
        if(circuit_cells->instance[i]->level > MaxCircuitLevel)
          {
            MaxCircuitLevel = circuit_cells->instance[i]->level;
          }
      }

    free(ReadyNets);
    free(ReadyGates);
  }

///////////////////////////////////////
////////     BDD FUNCTIONS    /////////
///////////////////////////////////////

double update_probability(char operator, double p_a, double p_b)
  { // Function to compute static probability of output pin from input pins

    double value;

    switch(operator)
      {
        case '*': // AND2
          value = p_a * p_b;
          value = (value < 0) ? 0 : (value > 1) ? 1 : value;
          return value;
        case '+': // OR2
          value = p_a + ((1 - p_a) * p_b);
          value = (value < 0) ? 0 : (value > 1) ? 1 : value;
          return value;
        case '^': // XOR2
          value = ((1 - p_a) * p_b) + ((1 - p_b) * p_a);
          value = (value < 0) ? 0 : (value > 1) ? 1 : value;
          return value;
        case '!': // INV
          value = 1 - p_a;
          value = (value < 0) ? 0 : (value > 1) ? 1 : value;
          return value;
        default: // BUF
          value = p_a;
          value = (value < 0) ? 0 : (value > 1) ? 1 : value;
          return value;
      }
      
  }

int isOperator(char ch)
  { // Function to check if a character is an operator
    
    return (ch == '!' || ch == '*' || ch == '+' || ch == '^');
  }

int getPrecedence(char op)
  { // Function to get the precedence of an operator
    
    if (op == '!')
      return 4;
    else if (op == '^')
      return 3;
    else if (op == '*')
      return 2;
    else if (op == '+')
      return 1;
    
    return 0;
  }

void print_dd(DdManager *gbm)
  { // Report information about the bdd
    
    printf("Ddmanager nodes:       %ld\n",Cudd_ReadNodeCount(gbm));
    printf("Ddmanager vars:        %d\n",Cudd_ReadSize(gbm));
    printf("DdManager memory:      %ld\n",Cudd_ReadMemoryInUse(gbm));
    printf("------------------------------------------\n");
  }

void infixToPostfix(char infix[], char postfix[])
  { // Function to convert infix expression to postfix using Shunting Yard algorithm

    int i, j;
    int top = -1;
    char stack[LEN_MAX];

    for (i = 0, j = 0; infix[i] != '\0'; i++)
      {
        char token = infix[i];

        if (isalnum(token))
          {
            postfix[j++] = token;
          }
        else if(token == '(')
          {
            stack[++top] = token;
          }
        else if(token == ')')
          {
            while(top >= 0 && stack[top] != '(')
              {
                postfix[j++] = ' '; // Space between operators
                postfix[j++] = stack[top--];
              }
            top--;
          }
        else if (isOperator(token)) 
          {
            while(top >= 0 && getPrecedence(stack[top]) >= getPrecedence(token) && stack[top] != '(')
              {
                postfix[j++] = ' '; // Space between operators
                postfix[j++] = stack[top--];
              }
            stack[++top] = token;
            postfix[j++] = ' ';
          } 
      }

    while (top >= 0)
      {
        postfix[j++] = ' '; // Space between operators
        postfix[j++] = stack[top--];
      }

    postfix[j] = '\0';
  }

DdNode *BddNode_creator(DdManager *manager, char var[])
  { // Function to create BDD for a variable
    
    return Cudd_bddNewVar(manager);
  }

void Bdd_remover(DdManager *mgr, DdNode *node)
  { // Function to remove BDD for a variable

    if (!Cudd_IsConstant(node))
      {
        //printf("yep\n");
        Bdd_remover(mgr, Cudd_T(node));
        Bdd_remover(mgr, Cudd_E(node));
      }

    Cudd_RecursiveDeref(mgr, node);
  }

ProbabilityNode *ProbNode_creator(DdManager *manager, double prob, char name[])
  { // Function to create BDD node with probability
    
    DdNode *new_instance = Cudd_bddNewVar(manager);
    ProbabilityNode *prob_node = malloc(sizeof(ProbabilityNode));
    if (prob_node == NULL)
      {
          printf("Error: ProbNode_creator malloc failed.\n");
          exit(EXIT_FAILURE);
      }
    
    prob_node->node = Cudd_bddNewVar(manager);
    prob_node->probability = prob;
    strcpy(prob_node->name, name);

    return prob_node;
  }

void ProbNode_remover(DdManager *mgr, ProbabilityNode *PbNode)
  { // Function to remove BDD node with probability

    Bdd_remover(mgr, PbNode->node);
    free(PbNode);
    PbNode = NULL;
  }

DdNode *performOperation(DdManager *manager, DdNode *op1, DdNode *op2, char operator)
  { // Function to perform BDD operation based on operator
    
    switch (operator)
      {
        case '+':
          return Cudd_bddOr(manager, op1, op2);
        case '*':
          return Cudd_bddAnd(manager, op1, op2);
        case '^':
          return Cudd_bddXor(manager, op1, op2);
        default:
          fprintf(stderr, "Unsupported operator: %c\n", operator);
          exit(EXIT_FAILURE);
      }
  }

DdNode *TemporaryBuildBDDFromPostfix(DdManager *manager, char *postfix)
  { // Function to build BDD from postfix expression
    
    int i, k;
    DdNode *stack[LEN_MAX];
    int top = -1;
    char varName[LEN_MAX];

    for (i = 0; postfix[i] != '\0'; i++)
      {
        char token = postfix[i];

        if (isalnum(token))
          {
            clean_str(varName);
            k = 0;

            while(isalnum(token) && token != ' ')
              {
                varName[k++] = token;
                token = postfix[++i];
              }
            varName[k] = '\0';
            
            stack[++top] = BddNode_creator(manager, varName);
          }
        else if (isOperator(token))
          {
            if(token == '!')
              {
                DdNode *operand = stack[top--];
                stack[++top] = Cudd_Not(operand);
                Cudd_Ref(stack[top]);
              }
            else
              {
                DdNode *op2 = stack[top--];
                DdNode *op1 = stack[top--];
                stack[++top] = performOperation(manager, op1, op2, token);
                Cudd_Ref(stack[top]);
              }
          }
      }

    return stack[top];
  }

ProbabilityNode *buildBDDFromPostfix(DdManager *manager, char *postfix, gatepin *gatepin_pointer)
  { // Function to build BDD from postfix boolean expression, while also using the already-built
    // BDDs of children of the root

    int i, k, j;
    ProbabilityNode *stack[LEN_MAX];
    int top = -1;
    char varName[LEN_MAX];
    component *component_pointer = find_component(circuit_cells, gatepin_pointer->owner);

    for (i = 0; postfix[i] != '\0'; i++)
      { // Initialize the current node of the BDD
         
        char token = postfix[i];

        if (isalnum(token))
          { // Operand found, store it

            clean_str(varName);
            k = 0;

            while(isalnum(token) && token != ' ')
              {
                varName[k++] = token;
                token = postfix[++i];
              }
            varName[k] = '\0';
            
            // Find current gatepin in component's input gatepins
            for(j = 0; j < component_pointer->in_count; j++)
              {
                if(!strcmp(component_pointer->cell_input[j]->name, varName))
                  { // Gatepin found

                    if(component_pointer->cell_input[j]->BDDTable.top == NULL)
                      {
                        component_pointer->cell_input[j]->BDDTable.top = ProbNode_creator(manager, component_pointer->cell_input[j]->static_probability, varName);
                      }
                    stack[++top] = component_pointer->cell_input[j]->BDDTable.top;
                  }
              }
          }
        else if (isOperator(token))
          { // Compute current operation in order

            if(token == '!')
              { // We take one operand node, sitting on top of the stack,
                // creating the probability node and updating its probability

                ProbabilityNode *curr_prob_node = stack[top--];
                char name[LEN_MAX];
                strcpy(name, curr_prob_node->name);
                DdNode *operand = curr_prob_node->node;
                stack[++top]->node = Cudd_Not(operand);
                Cudd_Ref(stack[top]->node);
                stack[top]->probability = update_probability(token, curr_prob_node->probability, 0.0);
                
                clean_str(stack[top]->name);
                stack[top]->name[0] = token;
                strcat(stack[top]->name, name);
              }
            else
              { // we take the two top operands of the stack, calculating the operation
                // and creating the new probability node.
                // We then update the probability of the new instance.

                ProbabilityNode *curr_prob_node_2 = stack[top--];
                char name2[LEN_MAX];
                strcpy(name2, curr_prob_node_2->name);
                ProbabilityNode *curr_prob_node_1 = stack[top--];
                char name1[LEN_MAX];
                strcpy(name1, curr_prob_node_1->name);
                DdNode *op2 = curr_prob_node_2->node;
                DdNode *op1 = curr_prob_node_1->node;
                
                stack[++top]->node = performOperation(manager, op1, op2, token);
                Cudd_Ref(stack[top]->node);
                stack[top]->probability = update_probability(token, curr_prob_node_1->probability, curr_prob_node_2->probability);
                
                clean_str(stack[top]->name);
                strcpy(stack[top]->name, name1);
                stack[top]->name[strlen(stack[top]->name)] = token;
                strcat(stack[top]->name, name2);
              }
          }
      }
    return stack[top];
  }

void BDDperInstance()
  { // Build BDD tree on every gate of the circuit, following the levelized hierarchy

    int i, j, k;
    component* comp_pointer;
    char postfix_expression[LEN_MAX];

    for(i = 1; i <= MaxCircuitLevel; i++)
      { // We parse the circuit level-by-level

        for(j = 0; j < circuit_cells->size; j++)
          {
            comp_pointer = circuit_cells->instance[j];
            if(comp_pointer->level == i)
              { // Matching level
              
                for(k = 0; k < comp_pointer->out_count; k++)
                  {
                    // We make the function to postfix format, in order to identify the type of gate
                    clean_str(postfix_expression);
                    infixToPostfix(comp_pointer->cell_output[k]->function, postfix_expression);
                    
                    // Initialize CUDD
                    comp_pointer->cell_output[k]->BDDTable.manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
                    comp_pointer->cell_output[k]->BDDTable.top = buildBDDFromPostfix(comp_pointer->cell_output[k]->BDDTable.manager, postfix_expression, comp_pointer->cell_output[k]);
                    comp_pointer->cell_output[k]->static_probability = comp_pointer->cell_output[k]->BDDTable.top->probability;
                  }
                for(k = 0; k < comp_pointer->CC_count; k++)
                  {
                    comp_pointer->CCs[k]->static_probability = comp_pointer->cell_output[0]->static_probability;
                  }
              }
          }
      }
  }

///////////////////////////////////////
///////     REPORT FUNCTIONS    ///////
///////////////////////////////////////

void list_components()
  { // Print all components
    
    int i, j;

    if(circuit_cells != NULL)
      {
        printf("------------------------------------------\n");
        for(i = 0; i < circuit_cells->size; i++)
          {
            printf("Component Name:     %s\n", circuit_cells->instance[i]->name);
            printf("--Cell Type:        %s\n", circuit_cells->instance[i]->cell_type);
            printf("--Cell Timing Type: %s\n", circuit_cells->instance[i]->cell_timing_type);
            printf("--Width:            %.3f\n", circuit_cells->instance[i]->dimensions[0]);
            printf("--Height:           %.3f\n\n", circuit_cells->instance[i]->dimensions[1]);
            printf("--Inputs:\n");
            for(j = 0; j < circuit_cells->instance[i]->in_count; j++)
              {
                printf("---- %s (/%s)\n", circuit_cells->instance[i]->cell_input[j]->owner, circuit_cells->instance[i]->cell_input[j]->name);
              }
            printf("--Outputs:\n");
            for(j = 0; j < circuit_cells->instance[i]->out_count; j++)
              {
                printf("---- %s (/%s), %s\n", circuit_cells->instance[i]->cell_output[j]->owner, circuit_cells->instance[i]->cell_output[j]->name, circuit_cells->instance[i]->cell_output[j]->function);
              }
            printf("--CCs:\n");
            for(j = 0; j < circuit_cells->instance[i]->CC_count; j++)
              {
                printf("---- %s (/%s)\n", circuit_cells->instance[i]->CCs[j]->owner, circuit_cells->instance[i]->CCs[j]->name);
              }
            printf("------------------------------------------\n");
          }
      }
  }

void list_IOs()
  { // Print all IOs
    
    int i, j;

    if(circuit_io != NULL)
      {
        printf("------------------------------------------\n");
        for(i = 0; i < circuit_io->size; i++)
          {
            printf("I/O Name: %s\n\n", circuit_io->instance[i]->name);
            printf("--CCs:\n");
            for(j = 0; j < circuit_io->instance[i]->CC_count; j++)
              {
                printf("---- %s (/%s)\n", circuit_io->instance[i]->CCs[j]->owner, circuit_io->instance[i]->CCs[j]->name);
              }
            printf("------------------------------------------\n");
          }
      }
  }

void report_component_func(char component_name[])
  { // Report the boolean function of a specific component
    
    int i;
    component *cc_pointer = find_component(circuit_cells, component_name);

    printf("------------------------------------------\n");
    if(cc_pointer == NULL)
      {
        printf("There is no component with name '%s'\n", component_name);
      }
    else
      {
        for(i = 0; i < cc_pointer->out_count; i++)
          {
            printf("Gatepin name: %s, Boolean Function: %s\n", cc_pointer->cell_output[i]->name, cc_pointer->cell_output[i]->function);
          }
      }
    printf("------------------------------------------\n");
  }

void report_component_type(char component_name[])
  { // Report the type (Sequential or Combinational) of a specific component
    
    component *cc_pointer = find_component(circuit_cells, component_name);
    printf("------------------------------------------\n");
    if(cc_pointer == NULL)
      {
        printf("There is no component with name '%s'\n", component_name);
      }
    else
      {
        printf("Component name: %s, Cell Timing Type: %s\n", cc_pointer->name, cc_pointer->cell_timing_type);
      }
    printf("------------------------------------------\n");
  }

void report_component_ccs(char component_name[])
  { // Report the connected components of a specific component
    
    int j;

    component *cc_pointer = find_component(circuit_cells, component_name);
    printf("------------------------------------------\n");
    if(cc_pointer == NULL)
      {
        printf("There is no component with name '%s'\n", component_name);
      }
    else
      {
        printf("Component name: %s\n", cc_pointer->name);
        printf("--CCs:\n");
        for(j = 0; j < cc_pointer->CC_count; j++)
          {
            printf("---- %s (/%s)\n", cc_pointer->CCs[j]->owner, cc_pointer->CCs[j]->name);
          }
      }
      printf("------------------------------------------\n");
  }

void report_component_gps(char component_name[])
  { // Report the connected gatepins of a specific component
    
    int j;

    component *cc_pointer = find_component(circuit_cells, component_name);
    printf("------------------------------------------\n");
    if(cc_pointer == NULL)
      {
        printf("There is no component with name '%s'\n", component_name);
      }
    else
      {
        printf("Component name: %s\n", cc_pointer->name);
        printf("\n--Inputs:\n");
        for(j = 0; j < cc_pointer->in_count; j++)
          {
            printf("---- %s (/%s)\n", cc_pointer->cell_input[j]->owner, cc_pointer->cell_input[j]->name);
          }
        printf("\n--Outputs:\n");
        for(j = 0; j < cc_pointer->out_count; j++)
          {
            printf("---- %s (/%s)\n", cc_pointer->cell_output[j]->owner, cc_pointer->cell_output[j]->name);
          }
      }
      printf("------------------------------------------\n");
  }

void report_all_component_gps()
  { // Report the connected gatepins of all components
    
    int i, j;

    for(j = 1; j <= MaxCircuitLevel; j++)
      {
        for(i = 0; i < circuit_cells->size; i++)
          {
            if(circuit_cells->instance[i]->level = j)
              report_component_gps(circuit_cells->instance[i]->name);
          }
      }
  }

void report_io_ccs(char io_name[])
  { // Report the connected components of an IO node
    
    int j;

    IO_node *io_pointer = find_io(circuit_io, io_name);
    printf("------------------------------------------\n");
    if(io_pointer == NULL)
      {
        printf("There is no I/O node with name '%s'\n", io_name);
      }
    else
      {
        printf("I/O name: %s\n", io_pointer->name);
        printf("--CCs:\n");
        for(j = 0; j < io_pointer->CC_count; j++)
          {
            printf("---- %s (/%s)\n", io_pointer->CCs[j]->owner, io_pointer->CCs[j]->name);
          }
      }
      printf("------------------------------------------\n");
  }

void report_lib_cell_BDD(char libcell_name[])
  { // Report the library cell's BDD

    int i = 0;
    char boolean_postfix[LEN_MAX];
    char outfile[LEN_MAX];
    DdManager *general_manager;

    // Initialize CUDD
    general_manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

    // Find Component
    component *cc_ptr = find_libcell(circuit_cells, libcell_name);
    for(i = 0; i < cc_ptr->out_count; i++)
      {
        infixToPostfix(cc_ptr->cell_output[i]->function, boolean_postfix);

        strcpy(outfile, "./dot_files/");
        strcat(outfile, cc_ptr->cell_output[i]->function);
        strcat(outfile, ".dot");

        // Build BDD
        DdNode *resultBDD = TemporaryBuildBDDFromPostfix(general_manager, boolean_postfix);
        Cudd_Ref(resultBDD);

        // Print BDD in DOT format
        FILE *dotFile = fopen(outfile, "w");
        if (dotFile == NULL)
          {
            fprintf(stderr, "Error opening dot file for writing.\n");
          }

        Cudd_DumpDot(general_manager, 1, &resultBDD, NULL, NULL, dotFile);

        Cudd_RecursiveDeref(general_manager, resultBDD);
        fclose(dotFile);

        // Display BDD information
        print_dd(general_manager);
      }
    
    // Clean up CUDD manager
    Cudd_Quit(general_manager);
  }

void report_component_BDD(char component_name[])
  { // Report the component's BDD

    int i = 0;
    char outfile[LEN_MAX];

    component *cc_ptr = find_component(circuit_cells, component_name);
    if(cc_ptr == NULL)
      {
        printf("No such component found.\n");
        return;
      }
    for(i = 0; i < cc_ptr->out_count; i++)
      {
        strcpy(outfile, "./dot_files/");
        strcat(outfile, cc_ptr->cell_output[i]->function);
        strcat(outfile, ".dot");

        // Print BDD in DOT format
        FILE *dotFile = fopen(outfile, "w");
        if (dotFile == NULL)
          {
            fprintf(stderr, "Error opening dot file for writing.\n");
          }

        Cudd_DumpDot(cc_ptr->cell_output[i]->BDDTable.manager, 1, &cc_ptr->cell_output[i]->BDDTable.top->node, NULL, NULL, dotFile);

        fclose(dotFile);

        // Display BDD information
        print_dd(cc_ptr->cell_output[i]->BDDTable.manager);
      }
  }

void report_gatepin_BDD(char component_name[], char gatepin_name[])
  { // Report the gatepin's BDD

    char outfile[LEN_MAX];

    component *cc_ptr = find_component(circuit_cells, component_name);
    if(cc_ptr == NULL)
      {
        printf("No such component found.\n");
        return;
      }
    gatepin *gp_pointer = find_gatepin(cc_ptr, gatepin_name);
    if(gp_pointer == NULL)
      {
        printf("No such gatepin found.\n");
        return;
      }
    
    strcpy(outfile, "./dot_files/");
    strcat(outfile, gp_pointer->function);
    strcat(outfile, ".dot");

    // Print BDD in DOT format
    FILE *dotFile = fopen(outfile, "w");
    if (dotFile == NULL)
      {
        fprintf(stderr, "Error opening dot file for writing.\n");
      }

    Cudd_DumpDot(gp_pointer->BDDTable.manager, 1, &gp_pointer->BDDTable.top->node, NULL, NULL, dotFile);

    fclose(dotFile);

    // Display BDD information
    print_dd(gp_pointer->BDDTable.manager);
  }

void compute_expression_BDD(char boolean_infix[])
  { // Compute a boolean expression's BDD
    
    char boolean_postfix[LEN_MAX];
    char outfile_dot[LEN_MAX];
    DdManager *general_manager;

    strcpy(outfile_dot, "./dot_files/");
    strcat(outfile_dot, boolean_infix);
    strcat(outfile_dot, ".dot");

    // Initialize CUDD
    general_manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

    infixToPostfix(boolean_infix, boolean_postfix);

    // Build BDD
    DdNode *resultBDD = TemporaryBuildBDDFromPostfix(general_manager, boolean_postfix);
    Cudd_Ref(resultBDD);

    // Print BDD in DOT format
    FILE *dotFile = fopen(outfile_dot, "w");
    if (dotFile == NULL)
      {
        fprintf(stderr, "Error opening dot file for writing.\n");
      }

    Cudd_DumpDot(general_manager, 1, &resultBDD, NULL, NULL, dotFile);

    Cudd_RecursiveDeref(general_manager, resultBDD);
    fclose(dotFile);

    // Display BDD information
    print_dd(general_manager);
    
    // Clean up CUDD manager
    Cudd_Quit(general_manager);
  }

int report_gp_level(char component_name[], char gp_name[])
  { // Function to report a gatepin's level
    
    int i, j;
    component *cell_pointer;
    gatepin *gp_pointer;

    cell_pointer = find_component(circuit_cells, component_name);

    printf("------------------------------------------\n");
    if (cell_pointer == NULL)
      {
        for(i = 0; i < circuit_io->size; i++)
          {
            for(j = 0; j < circuit_io->instance[i]->CC_count; j++)
              {
                if(!strcmp(circuit_io->instance[i]->CCs[j]->owner, component_name) && !strcmp(circuit_io->instance[i]->CCs[j]->name, gp_name))
                  {
                    printf("%s (/%s), level = %d\n", component_name, gp_name, circuit_io->instance[i]->CCs[j]->level);
                    printf("------------------------------------------\n");
                    return TCL_OK;
                  }
              }
          }

        printf("Could not find component with name '%s'.\n", component_name);
        printf("------------------------------------------\n");
        return(-1);
      }
    
    gp_pointer = find_gatepin(cell_pointer, gp_name);
    if(gp_pointer == NULL)
      {
        printf("Could not find gatepin '%s' in cell '%s'.\n", gp_name, component_name);
        printf("------------------------------------------\n");
        return(-1);
      }
    
    printf("%s (/%s), level = %d\n", gp_pointer->owner, gp_pointer->name, gp_pointer->level);
    printf("------------------------------------------\n");
    return TCL_OK;
  }

void report_level_gps(int searched_level)
  { // Function to report a level's gatepins
    
    int i, j;

    printf("------------------------------------------\n");
    printf("Level: %d\n", searched_level);
    printf("------------------------------------------\n");

    for(i = 0; i < circuit_cells->size; i++)
      {
        for(j = 0; j < circuit_cells->instance[i]->CC_count; j++)
          {
            if(circuit_cells->instance[i]->CCs[j]->level == searched_level)
              {
                printf("%s (/%s)\n", circuit_cells->instance[i]->CCs[j]->owner, circuit_cells->instance[i]->CCs[j]->name);
              }
          }
      }
    
    for(i = 0; i < circuit_io->size; i++)
      {
        for(j = 0; j < circuit_io->instance[i]->CC_count; j++)
          {
            if(circuit_io->instance[i]->CCs[j]->level == searched_level)
              {
                printf("%s (/%s)\n", circuit_io->instance[i]->CCs[j]->owner, circuit_io->instance[i]->CCs[j]->name);
              }
          }
      }
    
  }

void report_gps_levelized()
  { // Function to report the circuit's gatepins, sorted by level
    
    int i;

    for(i = 0; i <= MaxCircuitLevel; i++)
      {
        report_level_gps(i);
      }
  }

int report_component_level(char cc_name[])
  { // Function to report a component's level
    
    component *cc_pointer;

    printf("------------------------------------------\n");
    cc_pointer = find_component(circuit_cells, cc_name);
    if (cc_pointer == NULL)
    {
      printf("Could not find component with name '%s'.\n", cc_name);
      printf("------------------------------------------\n");
      return(-1);
    }
    
    printf("%s, level = %d\n", cc_name, cc_pointer->level);
    printf("------------------------------------------\n");

    return TCL_OK;
  }

void report_level_components(int searched_level)
  { // Function to report a level's components
    
    int i;

    printf("------------------------------------------\n");
    printf("Level: %d\n", searched_level);
    printf("------------------------------------------\n");

    for(i = 0; i < circuit_cells->size; i++)
      {
        if(circuit_cells->instance[i]->level == searched_level)
          {
            printf("%s\n", circuit_cells->instance[i]->name);
          }
      }
  }

void report_components_levelized()
  { // Function to report the circuit's components, sorted by level
    
    int i;

    for(i = 1; i <= MaxCircuitLevel; i++)
      {
        report_level_components(i);
      }
  }

void list_probabilities_all()
  { // Function to list static probabilities of all gatepins of the circuit

    int i, j, k;

    for(k = 1; k <= MaxCircuitLevel; k++)
      {
        printf("------------------------------------------\n");
        printf("Level: %d\n", k);
        printf("------------------------------------------\n");

        for(i = 0; i < circuit_cells->size; i++)
          {
            for(j = 0; j < circuit_cells->instance[i]->CC_count; j++)
              {
                if(circuit_cells->instance[i]->CCs[j]->level == k)
                  {
                    printf("%s (/%s), P = %.3lf\n", circuit_cells->instance[i]->CCs[j]->owner, circuit_cells->instance[i]->CCs[j]->name, circuit_cells->instance[i]->CCs[j]->static_probability);
                  }
              }
          }
      }
  }

void list_probabilities_chosen(char owner_name[], char gatepin_name[])
  { // Function to list static probability of given gatepin of the circuit

    component *component_pointer;
    gatepin *gatepin_pointer;

    component_pointer = find_component(circuit_cells, owner_name);
    if(component_pointer == NULL)
      {
        printf("Component %s was not found.\n", owner_name);
        return;
      }
    
    gatepin_pointer = find_gatepin(component_pointer, gatepin_name);
    if(gatepin_pointer == NULL)
      {
        printf("Gatepin %s (/%s) was not found.\n", owner_name, gatepin_name);
        return;
      }
    
    printf("------------------------------------------\n");
    printf("%s (/%s), P = %.3lf\n", owner_name, gatepin_name, gatepin_pointer->static_probability);
    printf("------------------------------------------\n");
  }

//////////////////////////////////////////////
/////    STATIC PROBABILITY FUNCTIONS    /////
//////////////////////////////////////////////

void set_probability_all(double probability_value)
  { // Function to set the given static probability to all startpoint gatepins of the circuit
    
    int i, j, k;
    IO_node *io_pointer;
    component *component_pointer;
    double computation_value;

    for (i = 0; i < circuit_io->size; i++)
      { // Setting static probability to all startpoints

        io_pointer = circuit_io->instance[i];
        if(io_pointer->io_type == PRIMARY_INPUT)
          {
            for(j = 0; j < io_pointer->CC_count; j++)
              {
                io_pointer->CCs[j]->static_probability = probability_value;
              }
          }
      }

    BDDperInstance();

    printf("All static probabilities have been updated.\n");
  }

void set_probability_chosen(char owner_name[], char gatepin_name[], double probability_value)
  { // Function to set the given static probability to a given gatepin of the circuit

    component *component_pointer;
    gatepin *gatepin_pointer;

    component_pointer = find_component(circuit_cells, owner_name);
    if(component_pointer == NULL)
      {
        printf("Component %s was not found.\n", owner_name);
        return;
      }
    
    gatepin_pointer = find_gatepin(component_pointer, gatepin_name);
    if(gatepin_pointer == NULL)
      {
        printf("Gatepin %s (/%s) was not found.\n", owner_name, gatepin_name);
        return;
      }
    
    gatepin_pointer->static_probability = probability_value;
    
    printf("Static probability of %s (/%s) has been updated.\n", owner_name, gatepin_name);
  }

//////////////////////////////////////
/////    READ DESIGN FUNCTION    /////
//////////////////////////////////////

void clear_circuit_parser()
  { // Free all dynamic memory allocated by the circuit parser

    int i,j;

    if(circuit_cells != NULL)
      {
        for(i = 0; i < circuit_cells->size; i++)
          { // Free memory stored by the circuit cells' information
            
            for(j = 0; j < circuit_cells->instance[i]->CC_count; j++)
              {
                if(circuit_cells->instance[i]->CCs[j]->BDDTable.manager != NULL)
                  { // Clear BDD information

                    ProbNode_remover(circuit_cells->instance[i]->CCs[j]->BDDTable.manager, circuit_cells->instance[i]->CCs[j]->BDDTable.top);
                    Cudd_Quit(circuit_cells->instance[i]->CCs[j]->BDDTable.manager);
                  }

                // Clear CCs
                free(circuit_cells->instance[i]->CCs[j]);
                circuit_cells->instance[i]->CCs[j] = NULL;
              }
            
            // Clear CC array
            free(circuit_cells->instance[i]->CCs);
            circuit_cells->instance[i]->CC_count = 0;

            // Clear Output gatepins' array
            circuit_cells->instance[i]->out_count = 0;
            free(circuit_cells->instance[i]->cell_output);
            circuit_cells->instance[i]->cell_output = NULL;
            
            // Clear Input gatepins' array
            circuit_cells->instance[i]->in_count = 0;
            free(circuit_cells->instance[i]->cell_input);
            circuit_cells->instance[i]->cell_input = NULL;
            
            // Clear component instance
            free(circuit_cells->instance[i]);
            circuit_cells->instance[i] = NULL;
          }
        
        // Clear components' array
        free(circuit_cells->instance);
        free(circuit_cells);
        circuit_cells = NULL;

        for(i = 0; i < circuit_io->size; i++)
          { // Free memory stored by the circuit I/Os' information
            
            if(circuit_io->instance[i]->io_type == PRIMARY_INPUT)
              {
                for(j = 0; j < circuit_io->instance[i]->CC_count; j++)
                  {
                    if(circuit_io->instance[i]->CCs[j] != NULL)
                      { // Clear CCs

                        if(circuit_io->instance[i]->CCs[j]->BDDTable.manager != NULL)
                          { // Clear BDD information

                            ProbNode_remover(circuit_io->instance[i]->CCs[j]->BDDTable.manager, circuit_io->instance[i]->CCs[j]->BDDTable.top);
                            Cudd_Quit(circuit_io->instance[i]->CCs[j]->BDDTable.manager);
                          }

                        free(circuit_io->instance[i]->CCs[j]);
                        circuit_io->instance[i]->CCs[j] = NULL;
                      }
                  }
              }
            
            // Clear CC array
            free(circuit_io->instance[i]->CCs);
            circuit_io->instance[i]->CCs = NULL;
            
            // Clear I/O instance
            free(circuit_io->instance[i]);
            circuit_io->instance[i] = NULL;
          }

        // Clear I/O' array
        free(circuit_io->instance);
        free(circuit_io);
        circuit_io = NULL;
      }
  }

int ReadDesign(const char filename[])
  { // Main function of reading a circuit design from a file

    FILE *parser_file; // File descriptor of practical format circuit design file
    int component_ht_capacity = 0;
    int io_ht_capacity = 0;

    parser_file = fopen(filename, "r");  //Open Verilog practical format circuit design file
    if (parser_file == NULL)
    {
        printf("Could not open Circuit Design file '%s' for reading.\n", filename);
        return(-1);
    }
    else
    {
        printf("Circuit Design file '%s' is open for reading.\n", filename);
    }

    // Clear circuit parser information
    clear_circuit_parser();

    // First parse: Calculate hash table capacities
    calculate_capacity(parser_file, &component_ht_capacity, &io_ht_capacity);

    // Second parse: Insert new information of circuit parser
    circuit_parser(parser_file, &component_ht_capacity, &io_ht_capacity);

    // Levelization of the circuit
    levelize_circuit();
    printf("Circuit parsing is done. Circuit Design file '%s' is closed.\n", filename);

    fclose(parser_file);
    return TCL_OK;
  }