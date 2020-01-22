#include "const.h"
#include "transplant.h"
#include "debug.h"

int validargs(int argc, char **argv);
int stringCompare(char *str1, char *str2);
int serialize();
int serialize_directory(int depth);
int serialize_file(int depth, off_t size);
int path_init(char *name);
int path_push(char *name);
int path_pop();
void printString(char *ch);
int find_length(char *input);
int getDepthBytes(int type);
int getSizeBytes(long size);
int deserialize();
int deserialize_directory(int depth);
int deserialize_file(int depth);
int checkMagic();
void clearNameBuff();
void addCharToNameBuff(char c);
int getDepth();
int getSize();
void getFileName();
void getFileName(int length);
int notSDPC(char *str);

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
	return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
	return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
	return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
	return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
	return "DIRECTORY_ENTRY";
    case FILE_DATA:
	return "FILE_DATA";
    default:
	return "UNKNOWN";
    }
}


int deserialize(){

    int depth, type;
    long size;

    //checking START_OF_TRANSMISSION
    if(checkMagic() == -1){
        return -1;
    };

    type = getchar();
    depth = getDepth();
    size = getSize();

    if(type == 0 && depth == 0 && size == 16){
        debug("passed SOT");
    }else{
        return -1;
    }


    //check START_OF_DIRECTORY
    if(checkMagic() == -1){
        return -1;
    }

    type = getchar();
    depth = getDepth();
    size = getSize();

    if(type == 2 && depth == 1 && size == 16){
        debug("passed SOD");
    }else{
        return -1;
    }

    DIR* dir = opendir(path_buf);
    if (dir) {
        closedir(dir);
    } else{
        mkdir(path_buf, 0700);
    }

    while(1){
        //get DEDIRECTORY_ENTRY data
        if(checkMagic() == -1){
            return -1;
        }

        int de_depth;
        type = getchar();
        de_depth = getDepth();
        size = getSize();

        //Not directory entry
        if(type == 4 && de_depth == depth && size >= 16){
            debug("NOT Directory Entry");
        }else if(type == 3 && de_depth == depth && size == 16 ){    //End of Directory
            debug("End of Directory");
            break;
        } else{
            return -1;
        }

        int st_mode;

        st_mode = getDepth();
        debug("st_mode %d", st_mode);

        //Just letting em out
        getSize();

        getFileName(size - 16 - 12); // Now Name buff contains File Name

        path_push(name_buf);

        debug("------------%s ---- %d", name_buf, st_mode);

        if(S_ISREG(st_mode)){
            // writefile
            deserialize_file(depth);
        }
        else if(S_ISDIR(st_mode)){
            deserialize_directory(depth);
        }
        else{
            return 0;
        }
    }

    //checking END_OF_TRANSMISSION
    if(checkMagic() == -1){
        debug("No EOT");
        return -1;
    };

    type = getchar();
    depth = getDepth();
    size = getSize();

    if(type == 1 && depth == 0 && size == 16){
        debug("passed EOT");
    }else{
        debug("didnt pass EOT");
        return -1;
    }

    return 0;
}

int deserialize_directory(int depth_DE){

    int depth, type;
    long size;

    //check START_OF_DIRECTORY
    if(checkMagic() == -1){
        return -1;
    }
    type = getchar();
    depth = getDepth();
    size = getSize();

    if(type == 2 && depth == depth_DE+1 && size == 16){
        debug("passed Start Of Directory");
    }else{
        debug("FAILED Start Of Directory");
        return -1;
    }


    mkdir(path_buf, 0700);

    while(1){
        //get DEDIRECTORY_ENTRY data
        if(checkMagic() == -1){
            return -1;
        }

        int de_depth;

        type = getchar();
        de_depth = getDepth();
        size = getSize();

        if(type == 4 && de_depth == depth && size >= 16){
            debug("NOT DE");
        }else if(type == 3 && de_depth == depth && size == 16 ){
            debug("End of Directory");
            break;
        } else{
            return -1;
        }

        // long st_size;
        int st_mode;

        st_mode = getDepth();

        //Spitting the charcters out
        getSize();

        getFileName(size - 16 - 12);
        // Now Name buff contains File Name

        path_push(name_buf);
        debug("------------%s ---- %d", name_buf, st_mode);

        if(S_ISREG(st_mode)){
            deserialize_file(depth);
        }
        else if(S_ISDIR(st_mode)){
            deserialize_directory(depth);
        }
        else{
            return 0;
        }
    }

    path_pop();


    return 0;
}

int deserialize_file(int depth_DE){

    int depth, type;
    long size;
    long data_length;

    if(checkMagic() == -1){
        return -1;
    }
    type = getchar();
    depth = getDepth();

    //16 + size
    size = getSize();

    if(depth == depth_DE && type == 5 && size >= 16){
        debug("passed FD");
    } else{
        return -1;
    }

    data_length = size - 16;

    struct stat   buffer;

    if(stat (path_buf, &buffer) == 0){
        if(global_options == 12){
            ;
        }else{
            debug("NO clobber permission");
            return -1;
        }
    }

    FILE *f = fopen(path_buf, "w");
    for(int i = 0; i < data_length; i++){
        fputc(getchar(), f);
    }

    fclose(f);
    path_pop();
    return 0;
}

void getFileName(int length){
    clearNameBuff();

    for(int i =0; i < length; i++){
        addCharToNameBuff(getchar());
    }
    debug("%s", name_buf);
}

int getDepth(){
    //4 bytes
    int depth = 256*256*256*getchar() + 256*256*getchar() + 256*getchar() + 1*getchar();
    return depth;
}

int getSize(){
    // 8 bytes
    int size = 0;
    for(int i = 0; i < 8; i++){
        int x = getchar();
        for(int j = 0; j < (7-i); j++){
            x = 256*x;
        }
        size += x;
    }
    return size;
}


int checkMagic(){

    clearNameBuff();

    if(getchar() != 0x0c){
        return -1;
    }
    if(getchar() != 0x0d){
        return -1;
    }
    if(getchar() != 0xed){
        return -1;
    }

    return 0;

}

void clearNameBuff(){
    int len = 0;
    while (*(name_buf+len)){
        *(name_buf+len) = '\0';
        ++len;
    }
}


int serialize_file(int depth, off_t size) {

    // FILE_DATA_HEADER
    int file_size;
    struct stat st;
    stat(path_buf, &st);
    file_size = st.st_size;

    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(FILE_DATA);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE + file_size);

    //FILE_DATA DATA
    FILE *f = fopen(path_buf, "r");
    int c;
    if(f){
        while ((c = getc(f)) != EOF)
            putchar(c);
    }

    fclose(f);
    path_pop();
    return 0;
}

int serialize(){

    int depth = 0;
    struct dirent *de;
    DIR *dir = opendir(path_buf);

    if(dir == NULL){
        debug("NO DIR\n");
        return -1;
    }

    //START_OF_TRANSMISSION
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(START_OF_TRANSMISSION);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE);

    depth++;

    //START OF DIR
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(START_OF_DIRECTORY);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE);


    //Loop till end of all DIR
    while((de = readdir(dir)) != NULL){

        debug("de->d_name: %s", de->d_name);

        if( stringCompare(de->d_name, ".") || stringCompare(de->d_name, "..")){
            continue;
        }
        else{
            path_push(de->d_name);

            struct stat stat_buf;
            off_t st_size;
            int st_mode;

            if (stat(path_buf, &stat_buf) != 0){
                //Error
                return -1;
            } else{
                st_size = stat_buf.st_size;
                st_mode = stat_buf.st_mode;
            }

            //DIRECTORY ENTRY
            // HEADER
            putchar(0x0C);
            putchar(0x0D);
            putchar(0xED);
            putchar(DIRECTORY_ENTRY);
            getDepthBytes(depth);
            getSizeBytes(16+12+find_length(de->d_name)); //totsize

            // METADATA
            getDepthBytes(st_mode);
            getSizeBytes(st_size); //filesize

            // filename
            printString(de->d_name);

            if (S_ISDIR(stat_buf.st_mode)){
                debug("Folder: %s\n", de->d_name);
                serialize_directory(depth+1);
            }
            else if (S_ISREG(stat_buf.st_mode)){
                debug("File %s\n", de->d_name);
                serialize_file(depth, st_size);
            } else{
                return -1;
            }
        }
    }

    //END_OF_DIRECTORY
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(END_OF_DIRECTORY);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE);


    depth--;
    //END_OF_TRANSMISSION
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(END_OF_TRANSMISSION);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE);

    closedir(dir);
    // fflush(stdout);
    return 0;
}

int getSizeBytes(long size){
    char byte1 = (size >> 56) & 0xFF;
    char byte2 = (size >> 48) & 0xFF;
    char byte3 = (size >> 40) & 0xFF;
    char byte4 = (size >> 32) & 0xFF;
    char byte5 = (size >> 24) & 0xFF;
    char byte6 = (size >> 16) & 0xFF;
    char byte7 = (size >> 8) & 0xFF;
    char byte8 = size & 0xFF;

    putchar(byte1);
    putchar(byte2);
    putchar(byte3);
    putchar(byte4);
    putchar(byte5);
    putchar(byte6);
    putchar(byte7);
    putchar(byte8);

    return 0;
}



int getDepthBytes(int type){

    char byte1 = (type >> 24) & 0xFF;
    char byte2 = (type >> 16) & 0xFF;
    char byte3 = (type >> 8) & 0xFF;
    char byte4 = type & 0xFF;

    putchar(byte1);
    putchar(byte2);
    putchar(byte3);
    putchar(byte4);

    return 0;

}

int serialize_directory(int depth){

    // START_OF_DIRECTORY
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(START_OF_DIRECTORY);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE);

    struct dirent *de;
    DIR *dir = opendir(path_buf);

    while((de = readdir(dir)) != NULL){

        debug("de->d_name: %s", de->d_name);
        if( stringCompare(de->d_name, ".") || stringCompare(de->d_name, "..")){
            continue;
        }
        else{

            path_push(de->d_name);

            struct stat stat_buf;
            off_t st_size;
            int st_mode;

            if (stat(path_buf, &stat_buf) != 0){
                //Error
                return -1;
            } else{
                st_size = stat_buf.st_size;
                st_mode = stat_buf.st_mode;
            }

            //DIRECTORY ENTRY
            // HEADER
            putchar(0x0C);
            putchar(0x0D);
            putchar(0xED);
            putchar(DIRECTORY_ENTRY);
            getDepthBytes(depth);
            getSizeBytes(16+12+find_length(de->d_name)); //totsize

            // METADATA
            getDepthBytes(st_mode);
            getSizeBytes(st_size); //filesize

            // filename
            printString(de->d_name);

            if (S_ISDIR(stat_buf.st_mode)){
                debug("Folder: %s\n", de->d_name);
                serialize_directory(depth+1);
            }
            else if (S_ISREG(stat_buf.st_mode)){
                debug("File %s\n", de->d_name);
                serialize_file(depth, st_size);
            } else{

            }
        }
    }

    // END_OF_DIRECTORY
    putchar(0x0C);
    putchar(0x0D);
    putchar(0xED);
    putchar(END_OF_DIRECTORY);
    getDepthBytes(depth);
    getSizeBytes(HEADER_SIZE);

    //END_OF_DIRECTORY
    closedir(dir);

    //popping path
    path_pop();
    return 0;
}


//TO print strings
void printString(char *ch){
  while(*ch) {
     putchar(*ch);
     ch++;
  }
}

// to find length of any string
int find_length(char *input){
    int length = 0;
    while(input[length]!='\0')
    {
        length++;
    }
    return length;
}

int validargs(int argc, char **argv){
    // To be implemented.
    if(argc == 1 || argc == 0){
        return -1;
    }
    else if(argc == 2){
        if(stringCompare(*(argv+1), "-h")){
            global_options = 0b0001;
            return 0;
        }
        else if(stringCompare(*(argv+1), "-s")){
            // serialization
            path_init(".");
            global_options = 0b0010;
            return 0;
        }
        else if(stringCompare(*(argv+1), "-d")){
            // deserialization
            path_init(".");
            global_options = 0b0100;
            return 0;
        }
        else{
            return -1;
        }
    }
    else if(argc == 3){
        if(stringCompare(*(argv+1), "-h")){
            global_options = 0b0001;
            return 0;
        }
        else if(stringCompare(*(argv+1), "-d") && stringCompare(*(argv+2), "-c")){
            // deserialization
            path_init(".");
            global_options = 0b1100;
            return 0;
        }
        else{
            return -1;
        }
    }
    else if(argc == 4){
        if(stringCompare(*(argv+1), "-h")){
            global_options = 0b0001;
            return 0;
        }
        else if(stringCompare(*(argv+2), "-p") && notSDPC(*(argv+3))){
            if(stringCompare(*(argv+1), "-s")){
                //serialize
                path_init(*(argv+3));
                global_options = 0b0010;
                return 0;
            }
            else if(stringCompare(*(argv+1), "-d")){
                //deserialize
                path_init(*(argv+3));
                global_options = 0b0100;
                return 0;
            }
            else{
                return -1;
            }
        }
        else{
            return -1;
        }
    }
    else if(argc == 5){
        if(stringCompare(*(argv+1), "-h")){
            global_options = 0b0001;
            return 0;
        }
        else if(stringCompare(*(argv+1), "-d")){
            if(stringCompare(*(argv+2), "-p") && stringCompare(*(argv+4), "-c") && notSDPC(*(argv+3))){
                global_options = 0b1100;
                path_init(*(argv+3));
                return 0;
            }
            else if(stringCompare(*(argv+2), "-c") && stringCompare(*(argv+3), "-p")  && notSDPC(*(argv+4))){
                global_options = 0b1100;
                path_init(*(argv+4));
                return 0;
            }
            else{
                return -1;
            }
        }
        else{
            return -1;
        }
    }
    else{
        if(stringCompare(*(argv+1), "-h")){
            global_options = 0b0001;
            return 0;
        }
        else{
            return -1;
        }
    }

    return -1;
}

int path_init(char *name) {
    int i=0;
    path_length = 0;
    while (*name != '\0'){
        *(path_buf+i) = *name++;
        i++;
        path_length++;
    }

    *(path_buf+i) = '\0';

    return -1;
}

// assumes that path_buf has been initialized to a valid * string.
int path_push(char *name){

    int i = path_length;

    //add '/'
    *(path_buf+i) = '/';
    path_length++;
    i++;

    while(*name != '\0'){
        *(path_buf+i) = *name++;
        i++;
        path_length++;
    }

    *(path_buf+i) = '\0';

    return -1;
}

int path_pop(){
    while(*(path_buf+path_length-1) != '/' && path_length != 0){
        *(path_buf+path_length-1) = '\0';
        path_length--;
    }

    *(path_buf+path_length-1) = '\0';
    path_length--;

    return -1;
}

int stringCompare(char *str1, char *str2){
    int ctr=0;

    while(*(str1+ctr)== *(str2+ctr))
    {
        if(*(str1+ctr)=='\0'||*(str2+ctr)=='\0')
            break;
        ctr++;
    }
    if(*(str1+ctr)=='\0' && *(str2+ctr)=='\0')
        return 1;
    else
        return 0;
}


//NOT -s or -d or -p or -c flags
int notSDPC(char *str){
    if(!stringCompare(str, "-s") && !stringCompare(str, "-d") && !stringCompare(str, "-p") && !stringCompare(str, "-c" )){
        return 1;
    }
    return 0;
}

//append char to end of name_buf
void addCharToNameBuff(char c){
    int len = 0;
    while (*(name_buf+len))
        ++len;

    name_buf[len] = c;
    name_buf[len+1] = '\0';
}
