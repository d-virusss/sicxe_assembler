#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

char input_file_name[50];
char output_file_name[50];
char label[15], opcode[15], operand[15], address[15];
int LOC, start_address;
int program_len;
int opcode_index, symbol_count;
int current_symbol_index;
int current_format;
int base_register = 0x0000;
int object_code = 0x000000;
char obj_text[10];
char text_record[150];
int text_count;
int text_starting_address;
int n, i, b, p, x, e, disp;
int label_loc;
int operand_label_flag = 0;

FILE* open_fp;
FILE* write_immediate;
FILE* read_immediate;
FILE* make_obj;
FILE* make_list;

void check();

struct OP_TABLE
{
  char name[8];
  int format;
  int opcode;
} opcode_table[] = {
    {"ADD", 3, 0x18}, {"ADDF", 3, 0x58}, {"ADDR", 2, 0x90}, {"AND", 3, 0x40}, {"CLEAR", 2, 0xB4}, {"COMP", 3, 0x28}, {"COMPF", 3, 0x88}, {"COMPR", 2, 0xA0}, {"DIV", 3, 0x24}, {"DIVF", 3, 0x64}, {"DIVR", 3, 0x9C}, {"FIX", 1, 0xC4}, {"FLOAT", 1, 0xC0}, {"HIO", 1, 0xF4}, {"J", 3, 0x3C}, {"JEQ", 3, 0x30}, {"JGT", 3, 0x34}, {"JLT", 3, 0x38}, {"JSUB", 3, 0x48}, {"LDA", 3, 0x00}, {"LDB", 3, 0x68}, {"LDCH", 3, 0x50}, {"LDF", 3, 0x70}, {"LDL", 3, 0x08}, {"LDS", 3, 0x6C}, {"LDT", 3, 0x74}, {"LDX", 3, 0x04}, {"LPS", 3, 0xD0}, {"MUL", 3, 0x20}, {"MULF", 3, 0x60}, {"MULR", 2, 0x98}, {"NORM", 1, 0xC8}, {"OR", 3, 0x44}, {"RD", 3, 0xD8}, {"RMO", 2, 0xAC}, {"RSUB", 3, 0x4C}, {"SHIFTL", 2, 0xA4}, {"SHIFTR", 2, 0xA8}, {"SIO", 1, 0xF0}, {"SSK", 3, 0xEC}, {"STA", 3, 0x0C}, {"STB", 3, 0x78}, {"STCH", 3, 0x54}, {"STF", 3, 0x80}, {"STI", 3, 0xD4}, {"STL", 3, 0x14}, {"STS", 3, 0x7C}, {"STSW", 3, 0xE8}, {"STT", 3, 0x84}, {"STX", 3, 0x10}, {"SUB", 3, 0x1C}, {"SUBF", 3, 0x5C}, {"SUBR", 2, 0x94}, {"SVC", 2, 0xB0}, {"TD", 3, 0xE0}, {"TIO", 1, 0xF8}, {"TIX", 3, 0x2C}, {"TIXR", 2, 0xB8}, {"WD", 3, 0xDC},
    {"START", 3, 5},
    {"END", 3, 2},
    {"BYTE", 3, 1},
    {"RESW", 3, 4},
    {"RESB", 3, 3},
    {"WORD", 3, 6},
    {"BASE", 3, 7}};

struct SYMBOL_TABLE{
  char label[10];
  int loc;
} symbol_table[200];

struct MODIFICATION_RECORD{
  char value[10];
} mod_record[100];

struct REGISTER{
  char name[5];
  int value;
} register_table[10] = {
    {"A", 0}, {"X", 1}, {"L", 2}, {"PC", 8}, {"SW", 9}, {"B", 3}, {"S", 4}, {"T", 5}, {"F", 6},
};

int modification[100];
int modification_count;

int search_opcode_table(char* word)
{
  opcode_index = 0;
  int e_flag = 0;
  for(int i=0; i< (sizeof(opcode_table) / sizeof(struct OP_TABLE)); i++){
    if(strcmp(word, opcode_table[i].name) == 0){
      return opcode_table[i].format;
    }
    else if(word[0] == '+'){
      e_flag = 1;
      opcode_index++;
    }
    else{
      opcode_index++;
    }
  }

  if(e_flag) return 4;
  return false;
}

int search_label_table(char* label){
  for(int i=0; i < symbol_count; i++){
    if(strcmp(label, symbol_table[i].label) == 0){
      current_symbol_index = i;
      return true;
    }
  }
  return false;
}

void search_register()
{
  char first_register[4], second_register[4];
  int exist_second = 0;
  memset(first_register, 0, sizeof(first_register));
  memset(second_register, 0, sizeof(second_register));
  char *word;
  word = strtok(operand, ", ");
  strcpy(first_register, word);
  word = strtok(NULL, " \t\n");
  if(word != NULL){
    strcpy(second_register, word);
    word = strtok(NULL, " \t\n");
    exist_second = 1;
  }

  for (int i = 0; i < 9; i++)
  {
    if (strcmp(first_register, register_table[i].name) == 0)
    {
      object_code += (register_table[i].value << 4);
    }
    if(exist_second){
      if (strcmp(second_register, register_table[i].name) == 0)
      {
        object_code += register_table[i].value;
      }
    }
  }
}

void parseData(char* line)
{
  char* word;
  current_format = 0;
  int tmp_num, anomaly_flag, format = 0;
  memset(label, 0, sizeof(label));
  memset(opcode, 0, sizeof(label));
  memset(operand, 0, sizeof(label));

  for(word = strtok(line, " \t\n"); word != NULL;)
  {
    tmp_num = atoi(word);
    if(!tmp_num) // 받아온 어절이 문자일 경우
    {
      for(int i = 0; i < strlen(word); i++){
        if(word[0] == '+') continue;
        if(!isalpha(word[i])){
          anomaly_flag = true;
          break;
        }
      } // 알파벳인지 기타문자인지 체크
      if(anomaly_flag == false){ // 알파벳일 경우에만 진행, comment인 경우에는 큰 for문 반복하며 빠져나감
        current_format = search_opcode_table(word);
        if (current_format > 0)
        { // opcode_table에 있는 instruction일 경우
          strcpy(opcode, word);
          if (current_format == 1 || current_format == 2)
          {
            word = strtok(NULL, " \t\n");
            strcpy(operand, word);
          }
          else if(strcmp(opcode, "RSUB") != 0){ //
            word = strtok(NULL, " \t\n");
            strcpy(operand, word);
            if(word[strlen(word)-1] == ','){
              word = strtok(NULL, " \t\n");
              strcat(operand, word);
            }
          }
        }
        else{ // word가 알파벳임에도 op_table에 없는경우는 label로 등록!
          strcpy(label, word);
          word = strtok(NULL, " \t\n");
          strcpy(opcode, word);
          word = strtok(NULL, " \t\n");
          strcpy(operand, word);
        }
      }
    }
    word = strtok(NULL, " \t\n");
  }
}

void cal_loc(){
  if(current_format == 4) LOC += 4;
  else if(current_format == 1) LOC += 1;
  else if(current_format == 2) LOC += 2;
  else if(current_format == 3){
    if(strcmp(opcode, "WORD") == 0){
      LOC += 3;
    }
    else if(strcmp(opcode, "RESW") == 0){
      LOC += 3 * atoi(operand);
    }
    else if(strcmp(opcode, "RESB") == 0){
      LOC += atoi(operand);
    }
    else if(strcmp(opcode, "BYTE") == 0){
      if(operand[0] == 'C'){
        LOC += strlen(operand) - 3; // C'S'의 경우 ''를 포함하여 총 4글자 이기 때문에, 2개의 '와 C를 카운트하지 않기 위하여 3을 감한다.
      }
      else if(operand[0] == 'X'){
        LOC += (strlen(operand) - 3) / 2; // X의 경우 1byte에 HEX 값을 저장하기 때문에 X'01'의 경우 1byte만 차지한다. 따라서 2개의 '와 X의 수인 3을 감하고 HEX 2개가 1byte이므로 2로 제한다.
      }
    }
    else if(strcmp(opcode, "BASE") == 0) LOC += 0;
    else LOC += 3;
  }
}


void pass1(){
  char buffer[100];
  memset(buffer, 0, sizeof(buffer));
  while (fgets(buffer, sizeof(buffer), open_fp) != NULL)
  {
    parseData(buffer);
    if(strcmp(opcode, "START") == 0){
      LOC = strtol(operand, NULL, 16);
      start_address = LOC;
      if(strcmp(label, "") != 0){
        strcpy(symbol_table[symbol_count].label, label);
        symbol_table[symbol_count].loc = LOC;
        symbol_count++;
      }
      continue;
    }
    if(strcmp(opcode, "END") != 0){
      if(buffer[0] == '.') continue;
      else{
        if(strcmp(label, "") != 0){ // label이 있는 경우
          if(search_label_table(label) == false){ // label 등록 안돼있는경우
            strcpy(symbol_table[symbol_count].label, label);
            symbol_table[symbol_count].loc = LOC;
            symbol_count++;
          }
          else{ // label이 symbol_table에 이미 있는 경우
            printf("label is already exist in symbol table...\n");
          }
        }
        current_format = search_opcode_table(opcode);
        if(current_format != 0) cal_loc();
      }
    }
    memset(buffer, 0, sizeof(buffer));
  }
  program_len = LOC - start_address;
}

void get_obj_byte()
{
  char* word;
  int sum = 0;
  char type[3], value[100];
  memset(type, 0, sizeof(type));
  memset(value, 0, sizeof(value));
  word = strtok(operand, "'");
  strcpy(type, word);
  word = strtok(NULL, "'");
  strcpy(value, word);
  word = strtok(NULL, " \t\n");
  if(type[0] == 'C'){
    if(strcmp(value, "EOF") == 0){
      object_code = 0x454F46;
    }
    else{
      for(int i=2; i<strlen(operand)-2; i++){
        if(i == strlen)
        sum += (int)operand[i];
        sum = sum << 8;
      }
      sum += operand[strlen(operand) - 2];
    }
    sprintf(obj_text, "%.*X", strlen(operand) - 3, object_code);
    strcat(text_record, obj_text);
    text_count++;
  }
  else if(type[0] == 'X'){
    strcat(text_record, value);
  }
}

void get_obj_word()
{
  char* word;
  object_code = strtoul(operand, NULL, 10);
}

bool check_use_x_register()
{
  if((operand[strlen(operand)-2] == ',' && operand[strlen(operand)-1] == 'X') ){
    return true;
  }
  return false;
}

void get_immediate_disp()
{
  char* word;
  operand_label_flag = 0;
  word = strtok(operand, "#");
  for(int i=0; i<strlen(word); i++){
    if(!isdigit(word[i])){ // 전부 숫자가 아니면 label로 판단
      operand_label_flag = 1;
    }
  }
  if(operand_label_flag){
    search_label_table(word);
    disp = symbol_table[current_symbol_index].loc;
  }
  else{ // 숫자일 경우에는 그대로 읽어서 disp에 저장, 10진수로 읽어야함!
    disp = strtol(word, NULL, 10);
  }
}

void cal_format3()
{
  int tmp;
  int binary = 0b111111111111;
  if(p==1 && disp < 0){
    disp = disp&binary;
  }
  object_code = opcode_table[opcode_index].opcode;
  object_code += n << 1;
  object_code += i;
  object_code = object_code << 16; // format3의 앞자리 hexa값 2개
  tmp = (x<<3) + (b<<2) + (p<<1) + e;
  tmp = tmp << 12;
  object_code += tmp;
  object_code += disp;
  sprintf(obj_text, "%.6X", object_code);
  strcat(text_record, obj_text);
  text_count++;
}

void get_format4_opcode()
{
  opcode_index = 0;
  char* word;
  word = strtok(opcode, "+");
  for (int i = 0; i < (sizeof(opcode_table) / sizeof(struct OP_TABLE)); i++){
    if (strcmp(word, opcode_table[i].name) == 0)
    {
      return;
    }
    else
    {
      opcode_index++;
    }
  }
}

void cal_format4()
{
  get_format4_opcode();
  object_code = opcode_table[opcode_index].opcode;
  object_code += n << 1;
  object_code += i;
  object_code = object_code << 24;
  object_code += ((x<<3) + e) << 20;
  object_code += disp;
  sprintf(obj_text, "%.8X", object_code);
  strcat(text_record, obj_text);
  text_count++;
}

void header_record()
{
  fprintf(make_obj, "H%-6s%06X%06X\n", label, strtol(operand, NULL, 16), program_len);
}

void T_record()
{
  fprintf(make_obj, "T%.6X%.2X%s\n", text_starting_address, strlen(text_record)/2, text_record);
  text_count = 0;
  memset(text_record, 0, sizeof(text_record));
}

void end_record()
{
  fprintf(make_obj, "E%.6X\n", start_address);
}

void write_mod_record()
{
  for(int i=0; i<modification_count; i++){
    fprintf(make_obj, "M%06X05\n", modification[i]);
  }
}

void get_flag_field()
{
  char* tmp;
  char word[15];
  memset(word, 0, sizeof(word));
  if(check_use_x_register()) x=1;
  if (operand[0] == '@'){
    n = 1;
    tmp = strtok(operand, "@");
    strcpy(word, tmp);
    if(search_label_table(word)){
      label_loc = symbol_table[current_symbol_index].loc;
      disp = label_loc - LOC;
    }
  }
  else if (operand[0] == '#'){
    i = 1;
    get_immediate_disp();
  }
  else n=1, i=1;
  
  if(n==1 && i==1 && x==0){
    if(search_label_table(operand)){
      label_loc = symbol_table[current_symbol_index].loc;
      disp = label_loc - LOC;
    }
  }
  if(n==1 && i==1 && x==1){
    tmp = strtok(operand, ",");
    strcpy(word, tmp);
    if(search_label_table(word)){
      label_loc = symbol_table[current_symbol_index].loc;
      disp = label_loc - LOC;
    }
  }
  if (disp > 2047 || disp < -2048){
    p=0, b=1;
    disp = label_loc - base_register;
    if (disp < 0 || disp > 4095){
      // format3 에서 relative addressing이 전부 안되므로 error 메시지 출력
    }
  }
  else if(n==0 && i==i && operand_label_flag == 0) p=0;
  else p=1;
}

void write_list_file()
{
  fprintf(make_list, "%04X\t\t%-10s\t%-10s\t\t%-15s\t\t\n", LOC, label, opcode, operand);
}

void pass2(){
  // 1. H record
  char buffer[100];
  text_starting_address = LOC;
  fgets(buffer, sizeof(buffer), open_fp);
  parseData(buffer);
  if(strcmp(opcode, "START") == 0){
    header_record();
    write_list_file();
  }
  while(fgets(buffer, sizeof(buffer), open_fp) != NULL)
  {
    label_loc = 0, n = 0, i = 0, b = 0, p = 0, x = 0, e = 0, disp = 0, current_format=0;
    object_code = 0x000000;
    memset(obj_text, 0, sizeof(obj_text));
    parseData(buffer);
    current_format = search_opcode_table(opcode);
    if(text_count == 0) text_starting_address = LOC;
    if(strcmp(opcode, "END") != 0 && buffer[0] != '.')
    {
      cal_loc();
      if(current_format == 1){
        object_code = opcode_table[opcode_index].opcode;
        sprintf(obj_text, "%2X", object_code);
        strcat(text_record, obj_text);
        text_count++;
      }
      else if(current_format == 2){
        object_code = opcode_table[opcode_index].opcode;
        object_code = object_code << 8;
        search_register();
        sprintf(obj_text, "%4X", object_code);
        strcat(text_record, obj_text);
        text_count++;
      }
      else if(current_format == 3){
        if(strcmp(opcode, "RESW") == 0 || strcmp(opcode, "RESB") == 0){
          if(text_count != 0) T_record();
        }
        else if(strcmp(opcode, "BYTE") == 0){
          get_obj_byte();
        }
        else if(strcmp(opcode, "WORD") == 0){
          get_obj_word();
          sprintf(obj_text, "%.6X", object_code);
          strcat(text_record, obj_text);
          text_count++;
        }
        else if(strcmp(opcode, "BASE") == 0){
          if(operand[0] == '#'){
            get_immediate_disp();
            base_register = disp;
          }
          else{
            search_label_table(operand);
            base_register = symbol_table[current_symbol_index].loc;
          }
        }
        else if(strcmp(opcode, "RSUB") == 0){
          n=1, i=1; // memory reference가 없으므로, x,b,p는 전부 0, format3이므로 e=0
          cal_format3();
        }
        else{
          get_flag_field();
          cal_format3();
        }
      }
      else if(current_format == 4){
        if(operand[0] == '@') n=i;
        else if(operand[0] == '#'){
          i=1;
          get_immediate_disp();
        }
        else n=1, i=1;
        if(check_use_x_register()) x=1;
        else x=0;
        e=1;
        if(n==1){
          if(search_label_table(operand)){
            disp = symbol_table[current_symbol_index].loc;
            modification[modification_count] = LOC - 3;
            modification_count++;
          }
        }
        cal_format4();
        if(n==0 && i ==1 && operand_label_flag == 1){
          modification[modification_count] = LOC -3;
          modification_count++;
        }
      }
      if(strlen(text_record) / 2 == 29){
        T_record();
      }
    }
    memset(buffer, 0, sizeof(buffer));
    write_list_file();
  }
  write_list_file();
  T_record();
  write_mod_record();
  end_record();
} // + listfile 생성하기

void read_immediate_line(char* line){
  char* word;
  int tmp_num = 0;
  memset(label, 0, sizeof(label));
  memset(address, 0, sizeof(label));

  for(word = strtok(line, " \t\n"); word != NULL;)
  {
    strcpy(label, word);
    word = strtok(NULL, " \t\n");
    strcpy(address, word);
    word = strtok(NULL, " \t\n");
  }
}

void get_info_of_immediate(){ // immediate.txt의 시작주소, 프로그램 길이, symbol table을 받아온다.
  memset(symbol_table, 0, sizeof(symbol_table));
  symbol_count = 0;
  char buffer[100];
  while(fgets(buffer, sizeof(buffer), read_immediate) != NULL)
  {
    read_immediate_line(buffer);
    if (strcmp(label, "START") == 0)
    {
      LOC = strtol(address, NULL, 16);
      start_address = LOC;
    }
    else if(strcmp(label, "P_LEN") == 0){
      program_len = strtol(address, NULL, 16);
    }
    else{
      if(search_label_table(label) == false){
        strcpy(symbol_table[symbol_count].label, label);
        symbol_table[symbol_count].loc = strtol(address, NULL, 16);
        symbol_count++;
      }
    }
    memset(buffer, 0, sizeof(buffer));
  }

}

void openfile(){
  open_fp = fopen(input_file_name, "r");
  if(open_fp == NULL){
    exit(1);
  }
  else{
    printf("open success!");
  }
}

void set_file_pointer(){
  read_immediate = fopen("immediate.txt", "r");
  if(read_immediate == NULL){
    printf("reading immediate file is failed!");
  }
  else{
    printf("reading immediate file is success!");
  }
  make_obj = fopen(output_file_name, "w");
  if(make_obj == NULL){
    printf("build stream to make object file is failed!");
  }
  else{
    printf("make obj stream success!");
  }
  make_list = fopen("assembly_list.txt", "w");
  if(make_list == NULL){
    printf("make_list stream build failed ! ");
  }
  else{
    printf("make_list stream build success!");
  }
}

void make_immediate(){
  fclose(open_fp);
  write_immediate = fopen("immediate.txt", "w");
  if(write_immediate == NULL){
    exit(1);
  }
  else{
    printf("make immediate success!");
  }

  fprintf(write_immediate, "START\t%X\n", start_address);
  fprintf(write_immediate, "P_LEN\t%X\n", program_len);
  for(int i=0; i<symbol_count; i++){
    fprintf(write_immediate, "%s\t%X\n", symbol_table[i].label, symbol_table[i].loc);
  }
  fclose(write_immediate);
}

void check(){
  for(int i=0; i<symbol_count; i++){
    printf("symbol[%d].label : %s\n", i, symbol_table[i].label);
    printf("symbol[%d].loc : %X\n", i, symbol_table[i].loc);
  }
  printf("start address is %X\n", start_address);
  printf("LOC is %X\n", LOC);
  printf("Program Length is %X\n", program_len);
  printf("symbol table print finish");
}

int main(int argc, char* argv[])
{
  strcpy(input_file_name, "copy.asm");
  strcpy(output_file_name, "result.txt");
  openfile();
  pass1();

  make_immediate();
  set_file_pointer();
  get_info_of_immediate(); // immediate에서 받은 정보들로 symbol table 구성
  
  openfile();
  pass2();

  printf("%s \n", input_file_name);
  printf("%s \n", output_file_name);

  return 0;
}