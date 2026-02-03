#ifndef ARGUMENT_PARSER_HPP_
#define ARGUMENT_PARSER_HPP_

#include <vector>
#include <string>
#include <cstring>
#include <map>

/**
 * Pythonのargparseをcppでほしい機能だけ再現したもの
 * ArgmentParser arg("program describe");
 * arg.add(
 *  "short name", // short argument name ex: -s
 *  "long name", // long argument name ex :--seed
 *  Typename, // argument [Typename] ex: INT, 
 *  *global_variable_pointer_to_sacve_argument, // 
 *  vector_size=n, // if use INT_VECTOR, DOUBLE_VECTOR
 *  describe="" // argument describe
 * );
 * 
 */

typedef enum TypeName{
    INT,
    FLOAT,
    DOUBLE,
    BOOL, // bool -option argument(true or false)
    CHAR,
    STRING, // string -option argument
    STRING_C, // const char* -option argument
    INT_PAIR, // pair<int,int> -option argument1 argument2
    DOUBLE_PAIR, // pair<double,double> -option argument1 argument2
    INT_VECTOR, // vector<int> -option arg1 arg2 arg3 ... argN,
    DOUBLE_VECTOR, // vector<double> -option arg1 arg2 arg3 ... argN
    INT_VECTOR_VECTOR, // vector<vector<int>> -option arg1 arg2 arg3 ... argN. add vector<int> value
    DOUBLE_VECTOR_VECTOR, // vector<vector<double>> -option arg1 arg2 arg3 ... argN. add vector<double> value
    FLAG // flag -option (no argument)
}TypeName;

class ArgumentParser{
public:
    struct Argument {
        const char* short_name;
        const char* long_name;
        TypeName type;
        void* value;
        const char* describe;
        int vector_size;
    };

    std::vector<Argument> arguments;
    const char* program_describe;

    ArgumentParser(const char* program_describe_string)
        : program_describe(program_describe_string){
        add("-h", "--help", TypeName::FLAG, nullptr, "show help option");
    }

    void add(const char* short_name, const char* long_name, TypeName type, void* value, const char* describe = "", int vector_size = 1){
        arguments.emplace_back(Argument{short_name, long_name, type, value, describe, vector_size});
    }

    // argument check if same short name or long name exists or not
    void check(){
        std::map<std::string, int> map;
        for (auto& arg : arguments) {
            if (map.find(arg.short_name) != map.end() || map.find(arg.long_name) != map.end()){
                fprintf(stderr, "Error: ArgumentParser: %s or %s is already exists.\n", arg.short_name, arg.long_name);
                exit(1);
            }
            map[arg.short_name] = 0; // value doesn't matter and add to map
            map[arg.long_name] = 0; // value doesn't matter and add to map
        }
    }
    
    void parse(int arg_c, char** arg_v){
        check();
        std::vector<int> v_itmp;
        std::vector<double> v_dtmp;
        for (int i = 1; i < arg_c; ++i){
            for (auto& arg : arguments) {
                if (strcmp(arg.short_name, arg_v[i]) == 0 || strcmp(arg.long_name, arg_v[i]) == 0){
                    if(i+1 >= arg_c && arg.type!=TypeName::FLAG){
                        fprintf(stderr, "ERROR at parse  Arg value not found : %s\n",arg_v[i]);
                        exit(0);
                    }
                    switch (arg.type){
                        case TypeName::INT:
                            *static_cast<int*>(arg.value) = atoi(arg_v[++i]);
                            break;
                        case TypeName::FLOAT:
                            *static_cast<float*>(arg.value) = (float)atof(arg_v[++i]);
                            break;
                        case TypeName::DOUBLE:
                            *static_cast<double*>(arg.value) = atof(arg_v[++i]);
                            break;
                        case TypeName::BOOL:
                            *static_cast<bool*>(arg.value) = strcmp(arg_v[++i], "true") == 0;
                            break;
                        case TypeName::CHAR:
                            *static_cast<char*>(arg.value) = arg_v[++i][0];
                            break;
                        case TypeName::STRING:
                            *static_cast<std::string*>(arg.value) = arg_v[++i];
                            break;
                        case TypeName::STRING_C:
                            *static_cast<const char**>(arg.value) = arg_v[++i];
                            break;
                        case TypeName::INT_PAIR:
                            *static_cast<std::pair<int, int>*>(arg.value) = std::make_pair(atoi(arg_v[i+1]), atoi(arg_v[i+2]));
                            i += 2;
                            break;
                        case TypeName::DOUBLE_PAIR:
                            *static_cast<std::pair<double, double>*>(arg.value) = std::make_pair(atof(arg_v[i+1]), atof(arg_v[i+2]));
                            i += 2;
                            break;
                        case TypeName::INT_VECTOR:
                            for(int itr=1;itr<=arg.vector_size;++itr){
                                (*static_cast<std::vector<int>*>(arg.value)).push_back(atoi(arg_v[i+itr]));
                            }
                            i += arg.vector_size;
                            break;
                        case TypeName::DOUBLE_VECTOR:
                            for(int itr=1;itr<=arg.vector_size;++itr){
                                (*static_cast<std::vector<double>*>(arg.value)).push_back(atof(arg_v[i+itr]));
                            }
                            i += arg.vector_size;
                            break;
                        case TypeName::INT_VECTOR_VECTOR:
                            if(strcmp("clear", arg_v[i+1]) == 0){
                                (*static_cast<std::vector<std::vector<int>>*>(arg.value)).clear();
                                break;
                            }
                            v_itmp.clear();
                            for(int itr=1;itr<=arg.vector_size;++itr){
                                v_itmp.push_back(atoi(arg_v[i+itr]));
                            }
                            (*static_cast<std::vector<std::vector<int>>*>(arg.value)).push_back(v_itmp);
                            i += arg.vector_size;
                            break;
                        case TypeName::DOUBLE_VECTOR_VECTOR:
                            if(strcmp("clear", arg_v[i+1]) == 0){
                                (*static_cast<std::vector<std::vector<double>>*>(arg.value)).clear();
                                break;
                            }
                            v_dtmp.clear();
                            for(int itr=1;itr<=arg.vector_size;++itr){
                                v_dtmp.push_back(atof(arg_v[i+itr]));
                            }
                            (*static_cast<std::vector<std::vector<double>>*>(arg.value)).push_back(v_dtmp);
                            i += arg.vector_size;
                            break;
                        case TypeName::FLAG:
                            if (strcmp(arg.short_name, "-h") == 0) {
                                describe();
                                exit(0);
                            }
                            *static_cast<bool*>(arg.value) = true;
                            break;
                    }
                }
            }
        }
    }

    void describe() const {
        fprintf(stderr, "%s\n",program_describe);
        for (const auto& arg : arguments){
            const char* type_name = "";
            switch (arg.type) {
                case TypeName::FLAG:
                    type_name = "flag";
                    break;
                case TypeName::INT:
                    type_name = "int";
                    break;
                case TypeName::FLOAT:
                    type_name = "float";
                    break;
                case TypeName::DOUBLE:
                    type_name = "double";
                    break;
                case TypeName::BOOL:
                    type_name = "bool";
                    break;
                case TypeName::CHAR:
                    type_name = "char";
                    break;
                case TypeName::STRING:
                    type_name = "string";
                    break;
                case TypeName::STRING_C:
                    type_name = "string_c";
                    break;
                case TypeName::INT_PAIR:
                    type_name = "int_pair";
                    break;
                case TypeName::DOUBLE_PAIR:
                    type_name = "double_pair";
                    break;
                case TypeName::INT_VECTOR:
                    type_name = "int_vector";
                    break;
                case TypeName::DOUBLE_VECTOR:
                    type_name = "double_vector";
                    break;
                case TypeName::INT_VECTOR_VECTOR:
                    type_name = "int_vector_vector";
                    break;
                case TypeName::DOUBLE_VECTOR_VECTOR:
                    type_name = "double_vector_vector";
                    break;
                
            }
            fprintf(stderr, "%s  %s  %s :\t%s\n", arg.short_name, arg.long_name, type_name, arg.describe);
        }
    }
};

#endif

/* 
// sample code
// parameters that will be changed by argument
bool use_gnuplot=false;
bool save_density=false;
int seed=1;
double L=1000.0;
double lambda=0.02;
double connection_range=10.0;
double velocity=1.0;
int main(int argc,char** argv){
    //useage
    ArgumentParser arg("test");
    arg.add("-s","--seed",INT,&seed,"simulation random seed");
    arg.add("-L","--L",DOUBLE,&L,"service area length");
    arg.add("-l","--lambda",DOUBLE,&lambda,"node poisson enter lambda");
    arg.add("-c","--connection_range",DOUBLE,&connection_range,"node connection range");
    arg.add("-v","--velocity",DOUBLE,&velocity,"node velocity");
    arg.add("-g","--use_gnuplot",FLAG,&use_gnuplot,"make gif by gnuplot");
    arg.add("-d","--save_density",FLAG,&save_density,"output node density");
    arg.parse(argc,argv);

    // arg -L {L} -l {lambda} -c {connection_range} -v {velocity} -g -d

    for(int i=0;i<argc;i++){
        printf("%s\n",argv[i]);
    }
    
    // result
    printf("seed = %d\n",seed);
    printf("L = %f\n",L);
    printf("lambda = %f\n",lambda);
    printf("connection_range = %f\n",connection_range);
    printf("velocity = %f\n",velocity);
    printf("use_gnuplot = %d\n",use_gnuplot);
    printf("save_density = %d\n",save_density);
    return 0;
}

 */