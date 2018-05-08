// version 2.5.
// A (not) fast,incremental version.



#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <ctime>
// C++ Header Files.
// Use iostream,fstream and sstream to find keywords and generate information.
// Algorithm and vector are used to simplify the coding process.

#include <dirent.h>
#include <string.h>
#include <magic.h>
#include <hiredis/hiredis.h>
#include <sys/stat.h>
// C Header Files.
// <dirent.h> : Used to traverse all files in the given directory.
// <magic.h> : Used to check the mime type of a given file.
// <hiredis.h> : Used to control redis by C++ code.

using std::cin;
using std::cout;
using std::count;
using std::sort;
using std::endl;
using std::vector;
using std::string;
using std::ifstream;
using std::ostringstream;
using std::to_string;
// Some objects which will be used.


namespace {

string setname;
string hashname;
char buffer[1000];
// setname is used to identify a set in redis.
// It's default value is "FileIndexer_result".

// This class is used to sort keywords by the number
// of their occurences in a file.
// Use it as a parameter of function sort.
class compare {
public:
  compare(vector<int> *a,int b) {
    lineserials=new vector<int>[b];
    for(int i=0;i<b;i++)
      lineserials[i]=a[i];
  }
  bool operator() (int a,int b) {
    return lineserials[a].size()>lineserials[b].size();
  }
private:
  vector<int> *lineserials;
};

// Use redis to store results.
void StoreInfo(const string& info) {
  redisContext *connect = redisConnect("127.0.0.1", 6379);
  // Function redisConnect : connect to the redis server
  if (connect != NULL && connect->err) {
      printf("connection error: %s\n", connect->errstr);
      return;
  } // error-handling
  redisCommand(connect,"SADD %s %s",setname.c_str(),info.c_str());
  redisFree(connect);
  // Connection closed.
  cout<<"Results saved!"<<endl;
}

// Get index information from redis
vector<string> GetInfo() {
  redisContext *connect = redisConnect("127.0.0.1", 6379);
  if (connect != NULL && connect->err) {
      printf("connection error: %s\n", connect->errstr);
      vector<string> nullstr;
      return nullstr;
  }
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(
    connect,
    "HKEYS %s",hashname.c_str())
  );
  // Function redisCommand : execute a redis command.
  // Its return value redisReply* is the pointer to a struct
  // which includes all infomation of this executed command.
  // (But it's user's job to transform its type to redisReply*)
  int size=reply->elements;
  // member elements : the amount of members in this set.
  vector<string> filenames;
  for(int i=0;i<size;i++)
    filenames.push_back(reply->element[i]->str);
  // member element[] : an array of redisReply objects.
  // member str : the string returned.
  freeReplyObject(reply);
  // Use freeReplyObject to free the RAM used by reply.
  redisFree(connect);
  return filenames;
}



void Save(const string& filename,vector<int> *lineserials,string *keywords,int argc) {
  string info=filename+" : ";
  // Use index sort to sort keywords.
  // (I'm not sure if index sort is the best choice here, but this doesn't affect much.)
  // (All for convinience.)
  int index[argc-2];
  for(int i=0;i<argc-2;i++)
    index[i]=i;
  sort(index,index+argc-2,compare(lineserials,argc-2));
  // Generate the output information by ordered format.
  for(int i=0;i<argc-2;i++) {
    if (lineserials[index[i]].size()==0) continue;
    info=info+keywords[index[i]]+"("+to_string(lineserials[index[i]].size())+"): line ";
    vector<int>::iterator ite = lineserials[index[i]].begin();
    while (!lineserials[index[i]].empty()){
      info=info+to_string(lineserials[index[i]][0])+" ";
      lineserials[index[i]].erase(ite);
      ite = lineserials[index[i]].begin();
    }
    info=info+"  ";
  }
  StoreInfo(info);
}

// Use magic lib to check the MIME type of a file.
bool CheckType(const string& filename,const magic_t &checker) {
  // only plain text would be indexed.
  return !strcmp(magic_file(checker,filename.c_str()),"text/plain");
}

// This function checks if a file has been modified after last execute. 
bool Modified(redisContext *connect,const string& filename) {
  bool flag=true;
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(
    connect,
    "HEXISTS %s %s",hashname.c_str(),filename.c_str())
  );
  // Firstly we have to confirm the existance of such file.
  // If not, this file must be new
  if(reply->integer) {
    freeReplyObject(reply);
    reply = static_cast<redisReply*>(redisCommand(
      connect,
      "HGET %s %s",hashname.c_str(),filename.c_str())
    );
    FILE *file;
    int fd;
    struct stat status;
    file=fopen(filename.c_str(),"r");
    if(!file) {
        cout<<filename<<" may be deleted from disk. Cannot open."<<endl;
        return false;
    }
    fd=fileno(file);
    // Use fstat to get file's status.
    fstat(fd,&status);
    // Check st_mtime to confirm the latest modifie's time.
    if(to_string(status.st_mtime)==reply->str)
      flag=false;
    fclose(file);
  }
  freeReplyObject(reply);
  return flag;
}


// This recursive function returns a vector
// in which user can get all filenames in a directory.
void SaveFiles(redisContext* connect,const string& root) {
  
  magic_t checker;
  checker=magic_open(MAGIC_MIME_TYPE);
  magic_load(checker,NULL);

  // Use method c_str to transform root from string to char*.
  // dir is the pointer to the root directory.
  DIR* dir=opendir(root.c_str());
  redisReply *reply;
  if (dir) {
    dirent *file=NULL;
    // Read all filenames in current dir using while loop.
    while((file=readdir(dir))!=NULL){
      // struct dirent's member d_type shows the type of the file.
      // if d_type is equal to 8,that means this file is a normal file.
      // if d_type is equal to 4,then this file is a directory.
      if(file->d_type==4) {
        // If this file is a directory:
        // Ignoring '.' and ".." dir is okay.
        if(strcmp(file->d_name,".")&&strcmp(file->d_name,"..")){
          sprintf(buffer,"%s/%s",root.c_str(),file->d_name);
          // Recursive part.Search all files in this dir
          // and store the return value in a new vector.
          string name=buffer;
          SaveFiles(connect,name);
        }
      }
      else if (file->d_type==8) {
        sprintf(buffer,"%s/%s",root.c_str(),file->d_name);
        string name=buffer;
        if(CheckType(name,checker)&&Modified(connect,name)) {
        // To implement the incremental index, store the file's latest modifie's time.
          FILE *file;
          int fd;
          long long time;
          struct stat status;
          file=fopen(name.c_str(),"r");
          fd=fileno(file);
          fstat(fd,&status);
          time=status.st_mtime;
          fclose(file);
          reply = static_cast<redisReply*>(redisCommand(
            connect,
            "HSET %s %s %ld",hashname.c_str(),name.c_str(),time)
          );
          freeReplyObject(reply);
        }
      }
    }
  }
  else {
      cout<<"Error: \""<<root<<"\" is not a accessable directory."<<endl;
      exit(4);
  }
  // Donot forget to closedir.
  closedir(dir);
  magic_close(checker);
}

// This method returns a vector in which users can
// get all lineserials where the keyword has appeared.
vector<int> FindKeywords(string content,const string& keyword) {
  // Position represents the position of the keyword in a string.
  string::size_type position=0;
  vector<int>lineserial;
  // find method of a string searchs for a keyword in this string
  // starting from <position> and returns the first position of keyword's appearance.
  // npos represents the search is over.
  while((position = content.find(keyword,position)) != string::npos){
    // Use an iterator to get the lineserial of the position.
    string::iterator ite=content.begin();
    // count method is in header <algorithm>.
    // It returns the amount of '\n' between two keyword,
    // so it can get the lineserial this way.
    int line=count(ite,ite+position,'\n')+1;
    lineserial.push_back(line);
    // Remenber to move the position foward to detect the next keyword.
    position++;
  }
  return lineserial;
}

vector<string> GetFiles(const string& root) {
  magic_t checker;
  checker=magic_open(MAGIC_MIME_TYPE);
  magic_load(checker,NULL);
  
  vector<string> filenames;
  // Use method c_str to transform root from string to char*.
  // dir is the pointer to the root directory.
  DIR* dir=opendir(root.c_str());
  if (dir) {
    dirent *file=NULL;
    // Read all filenames in current dir using while loop.
    while((file=readdir(dir))!=NULL){
      // struct dirent's member d_type shows the type of the file.
      // if d_type is equal to 8,that means this file is a normal file.
      // if d_type is equal to 4,then this file is a directory.
      if(file->d_type==8){
        string name("/");
        // Joint the root directory and file name together,
        // that's the entire name of a file.
        name=root+name+string(file->d_name);
        // If this file is a plain text then
        // add this filename to the vector.
        if(CheckType(name,checker))
          filenames.push_back(name);
      }
      else if(file->d_type==4){
        // If this file is a directory:
        // Ignoring '.' and ".." dir is okay.
        if(strcmp(file->d_name,".")&&strcmp(file->d_name,"..")){
          string name("/");
          name=root+name+string(file->d_name);
          // Recursive part.Search all files in this dir
          // and store the return value in a new vector.
          vector<string>files=GetFiles(name);
          // Use the insert method to join files in this dir
          // to the filenames vector.
          filenames.insert(filenames.end(),files.begin(),files.end());
        }
      }
      // Error-handling.
      else {
        cout<<"\""<<file->d_name<<"\" is not a regular file."<<endl;
      }
    }
  }
  else {
      cout<<"Error: \""<<root<<"\" is not a accessable directory."<<endl;
      exit(4);
  }
  // Donot forget to closedir.
  closedir(dir);
  magic_close(checker);
  return filenames;
}

}  //namespace

int main(int argc , char* argv[])
{
  setname="FileIndexer_result";
  hashname="FileIndexer_info";
  if(argc<2) {
    cout<<"Error: invalid input. Use --help to get usage."<<endl;
    return -1;
  }
  // help info
  if(strcmp(argv[1],"--help")==0) {
    // The only use of system method is here......is that OK?
    // I just think this way is more elegant.
    system("cat helper");
    return 0;
  }
  if(strcmp(argv[1],"--buildindex")==0) {
    // --buildindex:only save the index of the files
    if(argc!=3) {
      cout<<"Error: invalid input. Use --help to get usage."<<endl;
      return -1;
    }
    cout<<"Please enter the HASH name of the index to be built in redis:";
    cin>>hashname;
    cout<<"Please wait patiently if you're indexing a big folder;"<<endl;
    cout<<"This may take up to a minute..."<<endl;
    redisContext *connect = redisConnect("127.0.0.1", 6379);
    if (connect != NULL && connect->err) {
        printf("connection error: %s\n", connect->errstr);
        return 1;
    }
    SaveFiles(connect,argv[2]);
    cout<<"All files have been indexed successfully."<<endl;
    redisFree(connect);
    return 0;
  }
  vector<string> filenames;
  if(strcmp(argv[1],"--searchbyindex")==0) {
    // --searchbyindex:search in a stored index
    cout<<"Please enter the HASH name of the index in redis:";
    cin>>hashname;
    filenames=GetInfo();
    cout<<"Please enter the SET name of the output in redis:";
    cin>>setname;
  }
  else {
    // if not get index from redis,get index from argv.
    if(argc<=2) {
      cout<<"Error: invalid input. Use --help to get usage."<<endl;
      return -1;
    }
    cout<<"Please enter the SET name of the output in redis:";
    cin>>setname;
    // Get filenames and keywords from input.
    filenames=GetFiles(argv[1]);
  }
  string keywords[argc-2];
  for(int i=0;i<argc-2;i++)
    keywords[i]=argv[i+2];
  // Search for keywords in every single file.
  vector<string>::iterator ite = filenames.begin();
  while (!filenames.empty()) {
    string filename=filenames[0];
    filenames.erase(ite);
    ite = filenames.begin();
    // Read the file's content.
    // ifstream opens a file by its name and stores its handle.
    ifstream input(filename.c_str());
    if (input){
      // Change the filestream to stringstream using method rdbuf.
      ostringstream read;
      read<<input.rdbuf();
      // Store infomation of the stringstream in a string.
      string content(read.str());
      bool ifexist=false;
      vector<int> lineserials[argc-2];
      // Call functions to search and print.
      for(int i=0;i<argc-2;i++){
        lineserials[i]=FindKeywords(content,keywords[i]);
        if(!lineserials[i].empty())
          ifexist=true;
      }
      if(ifexist)
        Save(filename,lineserials,keywords,argc);
    }
    else {
        cout<<"Cannot open this file."<<endl;
        return -1; 
    }
  }
  return 0;
}
