#include "elevate.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// util functions
int isSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int iSDigit(char c)
{
  return c >= '0' && c <= '9';
}
// json types
enum types {
  UNKNOWN,
  NUMBER,
  STRING,
  OBJECT,
  ARRAY,
  BOOLEAN
};
// the structure can be thought of as a tree but every child also points to its parent and to its next sibling
typedef struct node {
  char *key;       // sting key of the object entry
  enum types type; // the type of the value
  void *value;     // pointer to the value that will be stored in the heap
  struct node *par;
  struct node *sib;
} jsonGraphNode;

// a stack for storing whether the parser reads inside an object or an array (0=array, 1=object)
typedef struct {
  char *path;
  int n;   // the index of the top element + 1
  int max; // the maximum value for the index before reallocation is needed
} pathType;

// given a the first child of a node and a key the function checks if that node's children have that key and returns it
jsonGraphNode *findOnJSON(char *key, jsonGraphNode *node)
{
  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      return node;
    }
    node = node->sib;
  }
  return NULL;
}
// initializes a node of the graph structure with the provided values
jsonGraphNode *init_jsonNode(char *key, enum types type, jsonGraphNode *parent, jsonGraphNode *sibling, void *value)
{
  jsonGraphNode *temp = malloc(sizeof(jsonGraphNode));
  if (temp == NULL) return NULL;
  else {
    temp->type = type;
    temp->value = value;
    temp->key = key;
    temp->par = parent;
    temp->sib = sibling;
    return temp;
  }
}

/**
 @brief
  a tail recursive function to parse the given json
 @param checkKey
  whether the parser should read a new key inside a json object
 @param diverg
  a divergence counter that is increased when keys that do exist in the provided graph are found
  the values of the graph are only read when diverg is 0.
 @param pathStack
  a pointer to the pathType stack that is going to be used.
 @param node
  The current node for which the value is being read or its children are checked for keys when checkKey is 1.
 @return
  1 if a parsing error was encountered.
  2 if EOF was encountered while reading.
  3 if memory allocation has failed.
 */
int parseJSON(int checkKey, int diverg, pathType *pathStack, jsonGraphNode *node)
{
  int ch;
  while (isSpace(ch = getchar())); // skipping whiteSpaces
  if (ch == EOF) return 2;
  if (checkKey) {
    if (ch != '\"') return 1; // keys are only stings
    if (diverg > 0) {         // if diverg is not 0 just parse the key and read its value(checkKey = 0)
      while ((ch = getchar()) != '\"' && ch != EOF)
        if (ch == '\\') getchar(); // escapes should be read with no check
      if (ch == EOF) return 2;
      while (isSpace(ch = getchar()));
      if (ch != ':') return 1;
      return parseJSON(0, diverg + 1, pathStack, node); // divergence should be increased
    }
    else {
      char *key = malloc(21 * sizeof(char)); // allocate the key to compare it in increments of 20
      if (key == NULL) return 3;
      int i = 0;
      int n = 1;
      while ((ch = getchar()) != '\"' && ch != EOF) {
        key[i++] = ch;
        if (i > 20 * n) { // reallocation logic
          n++;
          if (n > 10) {
            free(key);
            return 1;
          }
          key = realloc(key, (20 * n + 1) * sizeof(char));
          if (key == NULL) return 3;
        }
        if (ch == '\\') { // read espaced characters
          ch = getchar();
          key[i++] = ch;
        }
        if (i > 20 * n) { // reallocation logic
          n++;
          if (n > 10) {
            free(key);
            return 1;
          }
          key = realloc(key, (20 * n + 1) * sizeof(char));
          if (key == NULL) return 3;
        }
      }
      key[i] = '\0';
      jsonGraphNode *next = findOnJSON(key, node->value); // find the key in the children
      free(key);

      if (ch == EOF) return 2;
      while (isSpace(ch = getchar())); // skipping whiteSpaces
      if (ch != ':') return 1;
      if (next == NULL) {
        return parseJSON(0, diverg + 1, pathStack, node); // if the key was not found increase divergence and parse the value(so the graph will be ignored)
      }
      else {
        return parseJSON(0, diverg, pathStack, next); // if the key was found read and store its value, by providing the found node
      }
    }
  }
  else {                 // a value is being read
    if (ch == '{') {     // the value is an object
      if (diverg == 0) { // check the value type of the stored node
        if (node->type != OBJECT && node->type != UNKNOWN) return 1;
        if (node->type == UNKNOWN) node->type = OBJECT;
      }
      while (isSpace(ch = getchar()));
      // push 1 to the pathStack
      (pathStack->n)++;                    // increase index
      if (pathStack->n > pathStack->max) { // reallocation logic
        pathStack->max += 16;
        pathStack->path = realloc(pathStack->path, pathStack->max * sizeof(char));
        if (pathStack->path == NULL) return 3;
      }
      pathStack->path[pathStack->n - 1] = 1;
      if (ch == '\"') {                               // if the next non white space character is '"' then a key is to be read
        ungetc(ch, stdin);                            // unget the character
        return parseJSON(1, diverg, pathStack, node); // and read the key
      }
      else if (ch != '}') return 1; // the only other acceptable character is '}'
    }
    else if (ch == '[') { // the value is an array
      if (diverg == 0) {  // check the value type of the stored node
        if (node->type != ARRAY && node->type != UNKNOWN) return 1;
        if (node->type == UNKNOWN) node->type = ARRAY;
      }
      while (isSpace(ch = getchar()));
      if (ch == ',') return 1; // the ',' is not acceptable without a value before it
      // push 0 to the pathStack
      (pathStack->n)++;
      if (pathStack->n > pathStack->max) { // reallocation logic
        pathStack->max += 16;
        pathStack->path = realloc(pathStack->path, pathStack->max * sizeof(char));
        if (pathStack->path == NULL) return 3;
      }
      pathStack->path[pathStack->n - 1] = 0;
      if (diverg == 0) {           // when reading an array a new child should be appended to the array node for every value read if the children were not provided
        if (node->value == NULL) { // create a new node with unknown type if a predetermined structure was not provided
          jsonGraphNode *next = init_jsonNode("arrElem", UNKNOWN, node, NULL, NULL);
          if (next == NULL) return 3;
          node->value = next;
        }
        node = node->value;
      }
      if (ch != ']') { // if there is a value to be read
        ungetc(ch, stdin);
        if (diverg == 0) return parseJSON(0, diverg, pathStack, node); // read the value with the child provided in node
        return parseJSON(0, diverg + 1, pathStack, node);              // increase divergence
      }
    }
    else if (ch == '\"') { // if a string is read
      if (diverg > 0) {    // just parse the string
        while ((ch = getchar()) != '\"' && ch != EOF)
          if (ch == '\\') getchar();
        if (ch == EOF) return 2;
        diverg -= 1; // divergence should be decreased when an entrie or value has been fully read
        while (isSpace(ch = getchar()));
      }
      else {
        if (node->type != STRING && node->type != UNKNOWN) return 1; // check type
        if (node->type == UNKNOWN) node->type = STRING;
        char *value = malloc(51 * sizeof(char)); // allocate string for value in increments of 50
        if (value == NULL) return 3;
        int i = 0;
        int n = 1;
        while ((ch = getchar()) != '\"') {
          value[i++] = ch;
          if (i > 50 * n) { // reallocation logic
            n++;
            if (n > 10) {
              free(value);
              return 1;
            }
            value = realloc(value, (50 * n + 1) * sizeof(char));
            if (value == NULL) return 3;
          }
          if (ch == '\\') { // read escaped characters
            ch = getchar();
            value[i++] = ch;
          }
          if (i > 50 * n) { // reallocation logic
            n++;
            if (n > 10) {
              free(value);
              return 1;
            }
            value = realloc(value, (50 * n + 1) * sizeof(char));
            if (value == NULL) return 3;
          }
        }
        value[i] = '\0';
        if (ch == EOF) {
          free(value);
          return 2;
        }
        while (isSpace(ch = getchar()));
        node->value = value;                   // store the value in the graph
        if (pathStack->path[pathStack->n - 1]) // if the value read was part of an object entry backtrack to the parent node
          node = node->par;
        // if it was in an array it will the node will be handled when the ',' is read
      }
    }
    else if (iSDigit(ch) || ch == '-') { // parse numbers
      if (diverg == 0) {                 // check type
        if (node->type != NUMBER && node->type != UNKNOWN) return 1;
        if (node->type == UNKNOWN) node->type = NUMBER;
      }
      char *value = malloc(31 * sizeof(char)); // allocate memory for value in increments of 30
      if (value == NULL) return 3;
      value[0] = ch;
      int i = 1;
      int n = 1;
      ch = getchar();
      while (ch != ',' && !isSpace(ch) && ch != '}' && ch != ']' && ch != EOF) {
        value[i++] = ch;
        if (i > 30 * n) { // reallocation logic
          n++;
          if (n > 10) {
            free(value);
            return 1;
          }
          value = realloc(value, (30 * n + 1) * sizeof(char));
          if (value == NULL) return 3;
        }
        ch = getchar();
      }
      value[i] = '\0';
      if (ch == EOF) {
        free(value);
        return 2;
      }
      while (isSpace(ch)) ch = getchar(); // if the value loop ended on a space read all spaces left
      char *ptr;
      errno = 0;
      double num = strtod(value, &ptr);
      if (strlen(ptr) > 0 || errno == ERANGE) { // error handling
        free(value);
        return 1;
      }
      if (diverg == 0) {
        double *val = malloc(sizeof(double)); // allocate memory in the heap
        if (val == NULL) {
          free(value);
          return 3;
        }
        *val = num;
        node->value = val;                          // add it to the graph
        if (pathStack->path[pathStack->n - 1] == 1) // if the value read was part of an object entry backtrack to the parent node
          node = node->par;
      }
      else {
        diverg -= 1; // decrease divergence
      }
      free(value);
    }
    else if (ch == 't') {
      if (diverg == 0) {
        if (node->type != BOOLEAN && node->type != UNKNOWN) return 1; // check type
        if (node->type == UNKNOWN) node->type = BOOLEAN;
      }
      if (getchar() != 'r') return 1;
      if (getchar() != 'u') return 1;
      if (getchar() != 'e') return 1;
      if (diverg == 0) { // put value 1 in the graph
        int *val = malloc(sizeof(int));
        if (val == NULL) return 3;
        *val = 1;
        node->value = val;
        if (pathStack->path[pathStack->n - 1] == 1) // if the value read was part of an object entry backtrack to the parent node
          node = node->par;
      }
      else {
        diverg -= 1;
      }
      while (isSpace(ch = getchar()));
    }
    else if (ch == 'f') {
      if (diverg == 0) {
        if (node->type != BOOLEAN && node->type != UNKNOWN) return 1;
        if (node->type == UNKNOWN) node->type = BOOLEAN;
      }
      if (getchar() != 'a') return 1;
      if (getchar() != 'l') return 1;
      if (getchar() != 's') return 1;
      if (getchar() != 'e') return 1;
      if (diverg == 0) { // put value 0 in the graph
        int *val = malloc(sizeof(int));
        if (val == NULL) return 3;
        *val = 0;
        node->value = val;
        if (pathStack->path[pathStack->n - 1] == 1) // if the value read was part of an object entry backtrack to the parent node
          node = node->par;
      }
      else {
        diverg -= 1;
      }
      while (isSpace(ch = getchar()));
    }
    else if (ch == 'n') {
      if (getchar() != 'u') return 1;
      if (getchar() != 'l') return 1;
      if (getchar() != 'l') return 1;
      if (diverg == 0) { // put NULL in the graph
        node->value = NULL;
        if (pathStack->path[pathStack->n - 1] == 1)
          node = node->par;
      }
      else {
        diverg -= 1;
      }
      while (isSpace(ch = getchar()));
    }
    else return 1; // no recognised value
    while (pathStack->n > 0) {
      if (ch == ',') {
        if (pathStack->path[pathStack->n - 1] == 1) return parseJSON(1, diverg, pathStack, node); // read next key in the object
        else if (diverg > 0) return parseJSON(0, diverg + 1, pathStack, node);                    // read next value in the array and increase divergence again after it was reduced by the previous logic
        else {
          if (node->sib == NULL) { // if divergence is 0 and there is not a node already in the structure create a new sibling for the node that was read
            jsonGraphNode *next = init_jsonNode("arrElem", UNKNOWN, node->par, NULL, NULL);
            if (next == NULL) return 3;
            node->sib = next;
            return parseJSON(0, diverg, pathStack, node->sib);
          }
          else return parseJSON(0, diverg, pathStack, node->sib); // otherwise just provide the existing sibling
        }
      }
      else if (ch == ']') { // the array value has ended
        if (pathStack->path[pathStack->n - 1] != 0) {
          return 1;
        } // parsing error as the stack value indicates the parser is in an object
        (pathStack->n)--; // pop the last element
        if (diverg > 0) {
          diverg -= 1; // decrease divergence as the array value has been read
        }
        else {
          node = node->par;                           // backtrack from the child to the array node
          if (pathStack->path[pathStack->n - 1] == 1) // if inside an object backtrack again
            node = node->par;
        }
      }
      else if (ch == '}') { // the object value has ended
        if (pathStack->path[pathStack->n - 1] != 1) {
          return 1;
        } // parsing error as the stack value indicates the parser is in an array
        (pathStack->n)--; // pop the last element
        if (diverg > 0) {
          diverg -= 1; // decrease divergence as the object value has been read
        }
        else {
          if (pathStack->path[pathStack->n - 1] == 1) // if inside an object backtrack to the parrent
            node = node->par;
        }
      }
      else {
        return 1; // invalid character
      }
      if (pathStack->n > 0) // skip spaces
        while (isSpace(ch = getchar()));
    }
    return 0; // parsing has finished
  }
}
// function for freeing the graph from memory
void recFree(jsonGraphNode *root)
{
  if (root->sib != NULL) recFree(root->sib);
  if (root->value != NULL) {
    if (root->type == OBJECT || root->type == ARRAY) {
      recFree(root->value);
    }
    else {
      free(root->value);
    }
  }
  free(root);
}
// cunstructs the destinations array based on the list of nodes that are provided
int *constructDestsArray(jsonGraphNode *listNode, int n)
{
  errno = 0;
  int *arr = malloc(n * sizeof(int));
  if (arr == NULL) {
    errno = ENOMEM;
    return NULL;
  }
  int i = 0;
  while (listNode != NULL) {
    if (listNode->type != NUMBER || *(double *)(listNode->value) != (int)*(double *)(listNode->value) || i > n - 1 || (int)*(double *)(listNode->value) < 0) {
      free(arr);
      return NULL;
      // handle type errors and number of elements error
    }
    arr[i++] = (int)*(double *)(listNode->value); // store the values in the array
    listNode = listNode->sib;
  }
  return arr;
}

void sendToolResponse(calcData responseData, char *id, int time) // response of the elevate tool
{
  printf("{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"content\":"
         "[{\"type\":\"text\",\"text\":\"{\\\"finalStop\\\":%d,\\\"minCost\\\":%d,\\\"status\\\":0,\\\"message\\\":\\\"OK\\\",\\\"time\\\":%d}\"}],"
         "\"structuredContent\":{\"finalStop\":%d,\"minCost\":%d,\"status\":0,\"message\":\"OK\",\"time\":%d},\"isError\":false}}\n",
         id, responseData.lastStop, responseData.minCost, time, responseData.lastStop, responseData.minCost, time);
  if (responseData.minStops != NULL) free(responseData.minStops);
}
// core message mcp message handler , returns 1 when EOF has been reached or memory allocation of the graph and stack has failed
int handle_message()
{
  // constucting the graph that is going to be given to the parser to indicate the elements that should be stored
  jsonGraphNode *root = init_jsonNode(NULL, OBJECT, NULL, NULL, NULL);
  if (root == NULL) return 1;
  jsonGraphNode *jsonrpc = init_jsonNode("jsonrpc", STRING, root, NULL, NULL);
  if (jsonrpc == NULL) {
    free(root);
    return 2;
  }
  jsonGraphNode *id = init_jsonNode("id", UNKNOWN, root, jsonrpc, NULL);
  if (id == NULL) {
    free(root);
    free(jsonrpc);
    return 2;
  }
  jsonGraphNode *method = init_jsonNode("method", STRING, root, id, NULL);
  if (method == NULL) {
    free(root);
    free(jsonrpc);
    free(id);
    return 2;
  }
  jsonGraphNode *params = init_jsonNode("params", OBJECT, root, method, NULL);
  if (params == NULL) {
    free(root);
    free(jsonrpc);
    free(id);
    free(method);
    return 2;
  }
  root->value = params;

  jsonGraphNode *paramsName = init_jsonNode("name", STRING, params, NULL, NULL);
  if (paramsName == NULL) {
    recFree(root);
    return 2;
  }
  jsonGraphNode *paramsArguments = init_jsonNode("arguments", OBJECT, params, paramsName, NULL);
  if (paramsArguments == NULL) {
    recFree(root);
    free(paramsName);
    return 2;
  }
  params->value = paramsArguments;

  jsonGraphNode *numPeople = init_jsonNode("numPeople", NUMBER, paramsArguments, NULL, NULL);
  if (numPeople == NULL) {
    recFree(root);
    return 2;
  }
  jsonGraphNode *numStops = init_jsonNode("numStops", NUMBER, paramsArguments, numPeople, NULL);
  if (numStops == NULL) {
    recFree(root);
    free(numPeople);
    return 2;
  }
  jsonGraphNode *dests = init_jsonNode("dests", ARRAY, paramsArguments, numStops, NULL);
  if (dests == NULL) {
    recFree(root);
    free(numPeople);
    free(numStops);
    return 2;
  }
  jsonGraphNode *mode = init_jsonNode("mode", STRING, paramsArguments, dests, NULL);
  if (mode == NULL) {
    recFree(root);
    free(numPeople);
    free(numStops);
    free(dests);
    return 2;
  }
  paramsArguments->value = mode;

  // allocating the path stack
  pathType stack;
  stack.max = 16;
  stack.n = 0;
  stack.path = malloc(16 * sizeof(char));
  if (stack.path == NULL) {
    recFree(root);
    return 1;
  }

  int res = parseJSON(0, 0, &stack, root);
  if (res != 0) {
    if (res == 1) { // parse error was encountered
      if (id->type == STRING && id->value != NULL) {
        printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32700, \"message\": \"Parse error\"},\"id\": \"%s\"}\n", (char *)id->value);
      }
      else if (id->type == NUMBER && id->value != NULL && *(double *)(id->value) == (int)*(double *)(id->value)) {
        printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32700, \"message\": \"Parse error\"},\"id\": %d}\n", (int)*(double *)(id->value));
      }
      else {
        printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32700, \"message\": \"Parse error\"},\"id\": null}\n");
      }
      free(stack.path);
      recFree(root);
      return 0;
    }
    else if (res == 3) { // memory allocation has failed
      if (id->type == STRING && id->value != NULL) {
        printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\": \"%s\"}\n", (char *)id->value);
      }
      else if (id->type == NUMBER && id->value != NULL && *(double *)(id->value) == (int)*(double *)(id->value)) {
        printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\": %d}\n", (int)*(double *)(id->value));
      }
      else {
        printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\": null}\n");
      }
      free(stack.path);
      recFree(root);
      return 0;
    }
    else { // EOF was reached
      free(stack.path);
      recFree(root);
      return 1;
    }
  }

  if ((id->type == NUMBER && *(double *)(id->value) != (int)*(double *)(id->value))) { // id was floating point
    printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32600, \"message\": \"Invalid Request: invalid id\"},\"id\": null}\n");
  }
  else if (id->type != UNKNOWN && id->value != NULL) { // messages with no id are ignored
    char idStr[510];                                   // giving enough space for the id if its a string
    // constructing a sting for the id whatever the type
    if (id->type == STRING) {
      sprintf(idStr, "\"%s\"", (char *)id->value);
    }
    else if (id->type == NUMBER) {
      sprintf(idStr, "%d", (int)*(double *)(id->value));
    }

    // checking jsonrpc
    if (jsonrpc->value == NULL) {
      printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32600,\"message\":\"Invalid Request: missing jsonrpc\"},\"id\":%s}\n", idStr);
    }
    else if (strcmp(jsonrpc->value, "2.0") != 0) {
      printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32600,\"message\":\"Invalid Request: invalid jsonrpc\"},\"id\":%s}\n", idStr);
    }

    // checking method
    else if (method->value == NULL) {
      printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32600,\"message\":\"Invalid Request: missing method field\"},\"id\":%s}\n", idStr);
    }
    else if (strcmp(method->value, "initialize") == 0) {
      printf("{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"protocolVersion\":\"2025-06-18\",\"capabilities\":"
             "{\"experimental\":{},\"prompts\":{\"listChanged\":false},\"resources\":"
             "{\"subscribe\":false,\"listChanged\":false},\"tools\":{\"listChanged\":true}},\"serverInfo\":"
             "{\"name\":\"elevate-mcp-server\",\"version\":\"1.0.0\"}}}\n",
             idStr);
    }
    else if (strcmp(method->value, "tools/list") == 0) {
      printf("{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"tools\":[{\"name\":\"elevate\",\"description\":"
             "\"Compute the optimal elevator final stop and cost.\",\"inputSchema\":"
             "{\"properties\":{\"numPeople\":{"
             "\"description\":\"Number of people that want to use the elevator.\",\"type\":\"integer\"},"
             "\"numStops\":{\"description\":\"Maximum number of stops the elevator can make.\","
             "\"type\":\"integer\"},\"dests\":{\"description\":"
             "\"Destination stop of each passenger.\",\"items\":{\"type\":\"integer\"},\"type\":\"array\"},"
             "\"mode\":{\"default\":\"dp\",\"description\":"
             "\"Solving mode to be used: recurse, brute, memoize, or dp.\","
             "\"enum\":[\"recurse\",\"brute\",\"memoize\",\"dp\"],\"type\":\"string\"}},\"required\":"
             "[\"numPeople\",\"numStops\",\"dests\"],\"type\":\"object\"},\"outputSchema\":"
             "{\"description\":\"Result of the elevator optimization.\",\"properties\":{\"finalStop\":"
             "{\"description\":\"Chosen final elevator stop (highest floor the elevator visits).\","
             "\"type\":\"integer\"},"
             "\"minCost\":{\"description\":\"Total minimal walking cost for all passengers.\","
             "\"type\":\"integer\"},\"status\":{\"description\":\"0 on success, non-zero on error.\","
             "\"type\":\"integer\"},\"message\":{\"description\":"
             "\"Human-readable status or error message.\",\"type\":\"string\"},\"time\":"
             "{\"description\":\"Elapsed time for the computation in microseconds.\","
             "\"type\":\"integer\"}},\"required\":[\"finalStop\",\"minCost\",\"status\",\"message\",\"time\"],"
             "\"type\":\"object\"},\"_meta\":{\"_fastmcp\":{\"tags\":[]}}}]}}\n",
             idStr);
    }
    else if (strcmp(method->value, "tools/call") == 0) {

      // checking tool name
      if (paramsName->value == NULL) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid Request: missing tool name\"},\"id\":%s}\n", idStr);
      }
      else if (strcmp(paramsName->value, "elevate") != 0) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: invalid tool name\"},\"id\":%s}\n", idStr);
      }

      // checking numPoeple
      else if (numPeople->value == NULL) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: missing numPeople tool parameter\"},\"id\":%s}\n", idStr);
      }
      else if (numPeople->type != NUMBER || *(double *)(numPeople->value) != (int)*(double *)(numPeople->value) || (int)*(double *)(numPeople->value) < 0) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: numPeople tool parameter must be a non negative integer\"},\"id\":%s}\n", idStr);
      }

      // checking numStops
      else if (numStops->value == NULL) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: missing numStops tool parameter\"},\"id\":%s}\n", idStr);
      }
      else if (numStops->type != NUMBER || *(double *)(numStops->value) != (int)*(double *)(numStops->value) || (int)*(double *)(numStops->value) < 0) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: numStops tool parameter must be a non negative integer\"},\"id\":%s}\n", idStr);
      }

      // checking dests
      else if (dests->value == NULL) {
        printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: missing dests numStops tool parameter\"},\"id\":%s}\n", idStr);
      }
      else {
        // getting values
        int valNumPeople = (int)*(double *)(numPeople->value);
        int valNumStops = (int)*(double *)(numStops->value);
        int *valDests = constructDestsArray(dests->value, valNumPeople);
        if (errno == ENOMEM) {
          printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\":%s}\n", idStr);
        }
        else if (valDests == NULL) {
          printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: the dests tool parameter must have numPeople number of items of non negative integer type\"},\"id\":%s}\n", idStr);
        }
        else {
          struct timeval start, end;
          gettimeofday(&start, NULL); // getting start time
          int valNumFloors = 0;       // finding numFloors
          for (int i = 0; i < valNumPeople; i++) {
            if (valDests[i] > valNumFloors) {
              valNumFloors = valDests[i];
            }
          }
          elevData data = {valNumFloors, valDests, valNumPeople, valNumStops}; // constructing the data for the tool
          calcData res;
          res.minNumStops = 0; // initializing values that might not be set by the elevate funtions, depending on mode
          res.minStops = NULL;
          if (mode->value == NULL || strcmp(mode->value, "dp") == 0) {
            if (dp(data, &res, 0) == 1) {
              printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\":%s}\n", idStr);
            }
            else {
              gettimeofday(&end, NULL); // getting end time
              int seconds = end.tv_sec - start.tv_sec;
              int microseconds = end.tv_usec - start.tv_usec;
              int time = (seconds * 1000000) + microseconds;
              sendToolResponse(res, idStr, time);
            }
          }
          else if (strcmp(mode->value, "brute") == 0) {
            if (brute(data, &res) == 1) {
              printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\":%s}\n", idStr);
            }
            else {
              gettimeofday(&end, NULL); // getting end time
              int seconds = end.tv_sec - start.tv_sec;
              int microseconds = end.tv_usec - start.tv_usec;
              int time = (seconds * 1000000) + microseconds;
              sendToolResponse(res, idStr, time);
            }
          }
          else if (strcmp(mode->value, "memoize") == 0) {
            if (memoize(data, &res) == 1) {
              printf("{\"jsonrpc\": \"2.0\",\"error\": {\"code\": -32603, \"message\": \"Internal error: memory allocation failed\"},\"id\":%s}\n", idStr);
            }
            else {
              gettimeofday(&end, NULL); // getting end time
              int seconds = end.tv_sec - start.tv_sec;
              int microseconds = end.tv_usec - start.tv_usec;
              int time = (seconds * 1000000) + microseconds;
              sendToolResponse(res, idStr, time);
            }
          }
          else if (strcmp(mode->value, "recurse") == 0) {
            recurse(data, &res);
            gettimeofday(&end, NULL); // getting end time
            int seconds = end.tv_sec - start.tv_sec;
            int microseconds = end.tv_usec - start.tv_usec;
            int time = (seconds * 1000000) + microseconds;
            sendToolResponse(res, idStr, time);
          }
          else {
            printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32602,\"message\":\"Invalid params: invalid value for mode tool parameter\"},\"id\":%s}\n", idStr);
          }
          free(valDests);
        }
      }
    }
    else { // invalid method
      printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\": -32601,\"message\":\"Method '%s' not found\"},\"id\":%s}\n", (char *)method->value, idStr);
    }
  }
  free(stack.path);
  recFree(root);
  return 0;
}

int main()
{
  // needed for parsing messages from models
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  while (handle_message() != 1);
}