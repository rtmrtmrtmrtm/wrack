#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <ctype.h>
#include <assert.h>
 
// split at white-space boundaries, treating runs of
// white space as a single space.
std::vector<std::string>
split(const std::string &str)
{
  std::vector<std::string> v;
  std::string tmp;
  for(int si = 0; si < str.size(); si++){
    unsigned int c = str[si];
    if(isspace(c)){
      if(tmp.size() > 0)
        v.push_back(tmp);
      tmp.erase();
    } else {
      tmp.push_back(c);
    }
  }
  if(tmp.size() > 0)
    v.push_back(tmp);
  return v;
}

// turn "string" into "std::string"
const char *
xxtype(std::string ty)
{
  if(ty == "string"){
    return "std::string";
  } else {
    return ty.c_str();
  }
}

class Col {
private:
  std::string name_;
  std::string type_;
  
public:
  Col(std::string name, std::string type)
    : name_(name), type_(type)
  {
  }
  std::string name() { return name_; }
  std::string type() { return type_; }
};

// index in cols, or -1
int
findcol(std::vector<Col> cols, std::string name)
{
  for(int i = 0; i < cols.size(); i++){
    if(cols[i].name() == name){
      return i;
    }
  }
  return -1;
}

class Operator;

// the description of the data-flow graph.
std::map<std::string,Operator*> operators;

class Operator {
private:
  std::string name_;
  
public:
  Operator(std::string name) : name_(name) { }
  std::string name() { return name_; }
  void emit_write_header(int i);
  void emit_read_header();
  bool output(int i, Operator *&, int &);
  void call(Operator *th, int);
  virtual std::vector<Col> input_cols(int);

  virtual std::vector<Col> output_cols() = 0;
  virtual void emit() = 0;
  virtual std::vector<std::string> inputs() = 0;
  virtual bool stores_output() = 0;
};

std::vector<Col>
Operator::input_cols(int i)
{
  std::vector<std::string> iv = inputs();
  assert(i < iv.size());
  Operator *th = operators[iv[i]];
  assert(th);
  return th->output_cols();
}

// emit write_ function declaration.
// there's one per input for each Operator.
void
Operator::emit_write_header(int i)
{
  printf("void\n");
  printf("write_%s_%d(", name().c_str(), i);
  auto cc = input_cols(i);
  for(auto it = cc.begin(); it != cc.end(); ++it){
    if(it != cc.begin())
      printf(", ");
    std::string type = (*it).type();
    std::string name = (*it).name();
    if(type == "string")
      printf("const ");
    printf("%s %s%s", xxtype(type), type=="string"?"&":"", name.c_str());
  }
  printf(")");
}

void
Operator::emit_read_header()
{
  std::vector<Col> cc = output_cols();
  printf("bool\n");
  printf("read_%s(", name().c_str());
  for(int i = 0; i < cc.size(); i++){
    if(i == 0)
      printf("const ");
    printf("%s ", xxtype(cc[i].type()));
    printf("&");
    printf("%s", cc[i].name().c_str());
    if(i+1 < cc.size())
      printf(", ");
  }
  printf(")");
}

// return info about Operator's i'th output.
bool
Operator::output(int output_index, Operator * &thx, int &input_index)
{
  for(auto it = operators.begin(); it != operators.end(); it++){
    Operator *th = it->second;
    std::vector<std::string> iv = th->inputs();
    for(int i = 0; i < iv.size(); i++){
      if(iv[i] == name()){
        if(output_index == 0){
          thx = th;
          input_index = i;
          return true;
        }
        output_index -= 1;
      }
    }
  }
  return false;
}

// pass the current record downstream.
// assumes the columns are in variables named after the columns.
void
Operator::call(Operator *th, int input_index)
{
  printf("  write_%s_%d(", th->name().c_str(), input_index);

  std::vector<Col> cv = output_cols();
  for(int i = 0; i < cv.size(); i++){
    printf("%s", cv[i].name().c_str());
    if(i + 1 < cv.size())
      printf(", ");
  }

  printf(");\n");
}

class Base : public Operator {
private:
  std::vector<Col> cols_;
  
public:
  Base(std::string name,
       std::vector<std::string> cols,
       std::vector<std::string> types)
    : Operator(name)
  {
    assert(cols.size() == types.size());
    for(int i = 0; i < cols.size(); i++){
      if(findcol(cols_, cols[i]) >= 0){
        fprintf(stderr, "Base %s double-defined column %s\n", name.c_str(), cols[i].c_str());
        exit(1);
      }
      cols_.push_back(Col(cols[i], types[i]));
    }
  }
  bool stores_output() { return false; }
  std::vector<Col> output_cols() {
    return cols_;
  }
  std::vector<Col> input_cols(int i) {
    assert(i == 0);
    return output_cols();
  }
  std::vector<std::string> inputs() {
    std::vector<std::string> v;
    return v;
  }
  void emit() {
    emit_write_header(0);
    printf("\n{\n");

    for(int i = 0; ; i++){
      Operator *xth;
      int xi;
      if(output(i, xth, xi) == false)
        break;
      call(xth, xi);
    }

    printf("}\n");
  }
};

class Count : public Operator {
private:
  std::string upname_; // source Operator
  std::string upcol_; // identifier column name

public:
  Count(std::string name, std::string upname, std::string upcol)
    : Operator(name), upname_(upname), upcol_(upcol)
  { }
  bool stores_output() { return true; }
  std::string uptype(){
    // inherits upstream ID column's type.
    std::vector<Col> ups = input_cols(0);
    int ci = findcol(ups, upcol_);
    assert(ci >= 0);
    return ups[ci].type();
  }
  std::vector<Col> output_cols() {
    std::vector<Col> v;
    v.push_back(Col(upcol_, uptype()));
    v.push_back(Col("count", "int"));
    return v;
  }
  std::vector<std::string> inputs() {
    std::vector<std::string> v;
    v.push_back(upname_);
    return v;
  }
  void emit() {
    printf("std::unordered_map<%s,int> %s_data;\n", xxtype(uptype()), name().c_str());

    emit_write_header(0);
    printf("\n{\n");

    // printf("  int count = %s_data[%s];\n", name().c_str(), upcol_.c_str());
    // printf("  count += 1;\n");
    // printf("  %s_data[%s] = count;\n", name().c_str(), upcol_.c_str());

    //printf("  int &count = %s_data[%s];\n", name().c_str(), upcol_.c_str());
    //printf("  count += 1;\n");

    printf("  int count;\n");
    printf("  auto it = %s_data.find(%s);\n", name().c_str(), upcol_.c_str());
    printf("  if(it != %s_data.end()){\n", name().c_str());
    printf("    count = it->second;\n");
    printf("    count += 1;\n");
    printf("    it->second = count;\n");
    printf("  } else {\n");
    printf("    count = 1;\n");
    printf("    %s_data[%s] = count;\n", name().c_str(), upcol_.c_str());
    printf("  }\n");

    for(int i = 0; ; i++){
      Operator *xth;
      int xi;
      if(output(i, xth, xi) == false)
        break;
      call(xth, xi);
    }

    printf("}\n");

    // allow joins to query back into our table by primary key.
    emit_read_header();
    printf("\n{\n");
    printf("  auto it = %s_data.find(%s);\n", name().c_str(), upcol_.c_str());
    printf("  if(it != %s_data.end()){\n", name().c_str());
    printf("    count = it->second;\n");
    printf("    return true;\n");
    printf("  } else {\n");
    printf("    return false;\n");
    printf("  }\n");
    printf("}\n");
  }
};

class View : public Operator {
private:
  std::string upname_;
  
public:
  View(std::string name, std::string upname)
    : Operator(name), upname_(upname)
  {
  }
  bool stores_output() { return true; }
  std::vector<Col> output_cols() {
    // inherit column names and types from input.
    Operator *th = operators[upname_];
    std::vector<Col> upcols = th->output_cols();
    return upcols;
  }
  std::vector<std::string> inputs() {
    std::vector<std::string> v;
    v.push_back(upname_);
    return v;
  }
  void emit() {
    printf("struct %s_record {\n", name().c_str());
    std::vector<Col> cc = output_cols();
    for(int i = 1; i < cc.size(); i++){
      printf("  %s %s;\n", xxtype(cc[i].type()), cc[i].name().c_str());
    }
    printf("};\n");
    printf("std::unordered_map<%s,%s_record> %s_data;\n", xxtype(cc[0].type()), name().c_str(), name().c_str());
    
    emit_write_header(0);
    printf("\n{\n");

    printf("  struct %s_record r;\n", name().c_str());
    for(int i = 1; i < cc.size(); i++){
      printf("  r.%s = %s;\n", cc[i].name().c_str(), cc[i].name().c_str());
    }
    printf("  %s_data[%s] = r;\n", name().c_str(), cc[0].name().c_str());

    for(int i = 0; ; i++){
      Operator *xth;
      int xi;
      if(output(i, xth, xi) == false)
        break;
      call(xth, xi);
    }

    printf("}\n");

    // XXX multi-core reads from a view?

    emit_read_header();
    printf("\n{\n");
    printf("  auto it = %s_data.find(%s);\n", name().c_str(), cc[0].name().c_str());
    printf("  if(it != %s_data.end()){\n", name().c_str());
    for(int i = 1; i < cc.size(); i++){
      printf("    %s = it->second.%s;\n", cc[i].name().c_str(), cc[i].name().c_str());
    }
    printf("    return true;\n");
    printf("  } else {\n");
    printf("    return false;\n");
    printf("  }\n");
    printf("}\n");
  }
};

class Join : public Operator {
private:
  std::vector<std::string> upnames_;
  std::vector<std::string> upcols_;

public:
  Join(std::string name,
       std::string upname0, std::string upcol0,
       std::string upname1, std::string upcol1)
    : Operator(name)
  {
    upnames_.push_back(upname0);
    upnames_.push_back(upname1);
    upcols_.push_back(upcol0);
    upcols_.push_back(upcol1);
  }
  bool stores_output() { return false; }
  std::vector<Col> output_cols() {
    std::vector<Col> v;
    std::map<std::string,bool> already;
    for(int i = 0; i < upnames_.size(); i++){
      std::vector<Col> ups = input_cols(i);
      for(Col c : ups){
        if(already[c.name()] == false){
          already[c.name()] = true;
          v.push_back(c);
        }
      }
    }
    return v;
  }
  std::vector<std::string> inputs() {
    return upnames_;
  }
  Operator * upop(int i) { return operators[upnames_[i]]; }
  void emit() {
    //
    // for each input that doesn't store its output,
    // a table to store a complete copy of that input.
    //
    for(int i = 0; i < upnames_.size(); i++){
      if(upop(i)->stores_output() == false){
        printf("struct %s_record_%d {\n", name().c_str(), i);
        std::vector<Col> cc = input_cols(i);
        for(auto c : cc){
          printf("  %s %s;\n", xxtype(c.type()), c.name().c_str());
        }
        printf("};\n");
        printf("std::unordered_map<%s,%s_record_%d> %s_data_%d;\n", xxtype(cc[0].type()), name().c_str(), i, name().c_str(), i);
      }
    }

    //
    // a write_() function for each input.
    //
    for(int i = 0; i < upnames_.size(); i++){
      emit_write_header(i);
      printf("\n{\n");

      //
      // store the new input record.
      // XXX what if it is an update, not a new record?
      //
      if(upop(i)->stores_output() == false){
        printf("  struct %s_record_%d r;\n", name().c_str(), i);
        std::vector<Col> cc = input_cols(i);
        for(auto c : cc){
          printf("  r.%s = %s;\n", c.name().c_str(), c.name().c_str());
        }
        printf("  %s_data_%d[%s] = r;\n", name().c_str(), i, cc[0].name().c_str());
      }

      //
      // search the other inputs for records that match the join key.
      // XXX if it's an update (not a new record), need negative outputs...
      //

      for(int j = 0; j < upnames_.size(); j++){
        if(i == j)
          continue;
        std::vector<Col> cc = input_cols(j);
        bool isprimary = (upcols_[j] == cc[0].name());
        if(upop(j)->stores_output() && isprimary){
          printf("{\n");
          if(cc[0].name() != upcols_[i]){
            printf("      %s %s = %s;\n", xxtype(cc[0].type()), cc[0].name().c_str(),
                   upcols_[i].c_str());
          }
          for(int k = 1; k < cc.size(); k++){
            printf("      %s %s;\n", xxtype(cc[k].type()), cc[k].name().c_str());
          }
          printf("  bool yes = read_%s(", upnames_[j].c_str());
          for(int k = 0; k < cc.size(); k++){
            printf("%s", cc[k].name().c_str());
            if(k+1 < cc.size())
              printf(", ");
          }
          printf(");\n");
          printf("if(yes){\n");
        } else if(upop(j)->stores_output() && isprimary == false){
          // XXX ask input operator to fetch perhaps multiple rows.
          assert(0);
        } else if(isprimary){
          // the join key is the primary key.
          printf("  {\n");
          printf("  auto it = %s_data_%d.find(%s);\n", name().c_str(), j, upcols_[i].c_str());
          printf("  if(it != %s_data_%d.end()){\n", name().c_str(), j);
          for(auto c : cc){
            printf("      %s %s = it->second.%s;\n", xxtype(c.type()), c.name().c_str(), c.name().c_str());
          }
        } else {
          // join key is not the primary key.
          // do a complete table scan.
          // XXX index
          printf("  for(auto&& [first,second] : %s_data_%d){\n", name().c_str(), j);
          printf("    if(second.%s == %s){\n", upcols_[j].c_str(), upcols_[i].c_str());
          for(auto c : cc){
            printf("      %s %s = second.%s;\n", xxtype(c.type()), c.name().c_str(), c.name().c_str());
          }
        }
      }

      for(int i = 0; ; i++){
        Operator *xth;
        int xi;
        if(output(i, xth, xi) == false)
          break;
        call(xth, xi);
      }

      for(int j = 0; j < upnames_.size(); j++){
        if(i == j)
          continue;
        printf("    }\n");
        printf("  }\n");
      }
      
      printf("}\n");
    }
  }
};

void
add(Operator *th)
{
  if(operators.find(th->name()) != operators.end()){
    fprintf(stderr, "double definition of %s\n", th->name().c_str());
    exit(1);
  }
  operators[th->name()] = th;
}

//
// generate code.
//
// the overall strategy:
// one write_name_X function per Operator per input.
// arguments are columns of a single input record.
// that function calls downstream Operators' write_ functions.
//
void
emit()
{
  printf("#include <vector>\n");
  printf("#include <map>\n");
  printf("#include <unordered_map>\n");
  printf("#include <string>\n");

  // function declarations first, one per input per Operator.
  printf("\n");
  for(auto it = operators.begin(); it != operators.end(); ++it){
    Operator *th = it->second;
    std::vector<std::string> iv = th->inputs();
    for(int i = 0; i < iv.size(); i++){
      th->emit_write_header(i);
      printf(";\n");
    }
    if(th->stores_output()){
      th->emit_read_header();
      printf(";\n");
    }
  }

  // now the function bodies.
  for(auto it = operators.begin(); it != operators.end(); ++it){
    Operator *th = it->second;
    printf("\n");
    th->emit();
  }
}

int
main(int argc, char *argv[])
{
  setbuf(stdout, 0);
  
  std::string line;
  while(std::getline(std::cin, line)){
    std::vector<std::string> v = split(line);

    if(v.size() < 2)
      continue;

    // base name colname1 type1 colname2 type2 ...
    // join name name1 col1 name2 col2
    // count name name col
    // view name name

    if(v[0] == "base"){
      std::string name = v[1];
      std::vector<std::string> colnames;
      std::vector<std::string> coltypes;
      for(int i = 2; i + 1 < v.size(); i += 2){
        colnames.push_back(v[i+0]);
        coltypes.push_back(v[i+1]);
      }

      Base *th = new Base(name, colnames, coltypes);
      add(th);
    } else if(v[0] == "count" && v.size() == 4){
      std::string name = v[1];
      std::string upname = v[2];
      std::string upcol = v[3];
      Count *th = new Count(name, upname, upcol);
      add(th);
    } else if(v[0] == "view" && v.size() == 3){
      std::string name = v[1];
      std::string upname = v[2];
      View *th = new View(name, upname);
      add(th);
    } else if(v[0] == "join" && v.size() == 6){
      std::string name = v[1];
      std::string upname0 = v[2];
      std::string upcol0 = v[3];
      std::string upname1 = v[4];
      std::string upcol1 = v[5];
      Join *th = new Join(name, upname0, upcol0, upname1, upcol1);
      add(th);
    } else {
      fprintf(stderr, "unknown directive %s\n", v[0].c_str());
    }
  }

  emit();
}
