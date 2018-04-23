#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
// C++ Header Files.
// Use iostream,fstream and sstream to find keywords and generate information.
// Algorithm and vector are used to simplify the coding process.

#include <dirent.h>
#include <string.h>
// C Header Files.
// Used to traverse all files in the given directory.

using std::cout;
using std::count;
using std::sort;
using std::endl;
using std::vector;
using std::string;
using std::ifstream;
using std::ostringstream;
// Some objects which will be used.


// This recursive function returns a vector
// in which user can get all filenames in a directory.
vector<string> GetFiles(string root) {
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
        // Add this filename to the vector.
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
  return filenames;
}

// This method returns a vector in which users can
// get all lineserials where the keyword has appeared.
vector<int> FindKeywords(string content,string keyword) {
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

// This method prints the information.
void Print(string keyword,vector<int> lineserial) {
  if (lineserial.size()==0) return;
  cout<<keyword<<"("<<lineserial.size()<<")"<<": line ";
  vector<int>::iterator ite = lineserial.begin();
  while (!lineserial.empty()){
    cout<<lineserial[0]<<" ";
    lineserial.erase(ite);
    ite = lineserial.begin();
  }
  cout<<"  ";
}

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


int main(int argc , char* argv[])
{
  if(argc<2) {
    cout<<"Error: invalid input. Use --help to get usage."<<endl;
    return -1;
  }
  // help info
  if(strcmp(argv[1],"--help")==0) {
    cout<<endl;
    return 0;
  }
  if(argc<=2) {
    cout<<"Error: invalid input. Use --help to get usage."<<endl;
    return -1;
  }
  // Get filenames and keywords from input.
  vector<string> filenames=GetFiles(argv[1]);
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
      if(ifexist){
        cout<<filename<<" : ";
        // Use index sort to sort keywords.
        int index[argc-2];
        for(int i=0;i<argc-2;i++)
          index[i]=i;
        sort(index,index+argc-2,compare(lineserials,argc-2));
        for(int i=0;i<argc-2;i++)
            Print(keywords[index[i]],lineserials[index[i]]);
        cout<<endl;
      }
    }
    else {
        cout<<"Cannot open this file."<<endl;
        return -1; 
    }
  }
  return 0;
}
