// Test Harness for Registry Plugin
#include <windows.h>
#include <stdio.h>

#include "registry/reg_entry.h"
#include "registry/reg_config.h"





int main(){
if(!init_registry()){printf("Init Registry Fail!\n");return -1;}
// TODO: MAKE SURE TO ANONYMIZE THE KEY PATH
unsigned char buffer[1024] = {0x00};
unsigned int buffer_len = sizeof(buffer);
char key_path[1024] = {0x00};
char value_name[128] = {0x00};
strcpy(key_path,"\\registry\\user\\software\\surreal\\riot engine");
strcpy(value_name,"poop");
if(!get_registry_value(key_path,value_name,buffer,&buffer_len)){
    printf("Get Value Failed!\n");
    return -1;
}
for(int i=0;i<buffer_len;i++){
    printf("%02x",buffer[i]);
}
printf("\n");

/*
char test_key_path[1024] = {0x00};
strcpy(test_key_path,"\\registry\\user\\software\\surReal\\riot engine");
printf("Key Path Exists: %d\n",key_path_exists(test_key_path));

strcpy(test_key_path,"\\registry\\user\\S-1-5-21-1431320325-1297723084-2957387153-1001\\SOFTWARE\\Hex-Rays\\IDA\\History");
printf("Anonymize Key Path Test: %s\n",test_key_path);

printf("Anonymize Key Path Test Result: %s\n",test_key_path);
char test_key_path_2[1024] = {0x00};
strcpy(test_key_path_2,"\\registry\\user\\software\\surreal\\riot engine");
char test_name[1024] = {0x00};
strcpy(test_name,"test_val");
char test_val[32] = {0x00};
DWORD wat = 666;
strcpy(test_val,"banana!");
create_registry_entry(test_key_path_2, test_name, 0, REG_DWORD, &wat, 4);
int reg_type = iniparser_getint(config,"\\registry\\user\\software\\surreal\\riot engine\\settings101:type",-1);
if(reg_type == -1){
    printf("Key Not Found\n");
    return -1;
}



if(iniparser_set(config,"\\registry\\user\\software\\surreal\\riot engine\\poop",NULL)){
    printf("Set Key Failed!\n");
    return -1;
}

if(iniparser_set(config,"\\registry\\user\\software\\surreal\\riot engine\\poop:type","1")){
    printf("Set Key Failed!\n");
    return -1;
}
if(iniparser_set(config,"\\registry\\user\\software\\surreal\\riot engine\\poop:data","HEYNOW")){
    printf("Set Key Failed!\n");
    return -1;
}




iniparser_dump_ini(config,fp);
dictionary_dump(config, stdout);

fclose(fp);
*/
cleanup_registry();
}

