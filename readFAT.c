#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE* myfile;
uint8_t boot_array[512];
int lastCluster[8] = {4088, 4089, 4090, 4091, 4092, 4093, 4094, 4095}; // this array has all the values that represent cluster in use and the last one in the file.
uint8_t* FAT;

	int sector_size;
	int cluster_size;
	int root_directory_size;
	int sectors_per_fat;
	int reserved_sector;
	int hidden_sector;
	int fat_offset_sector;
	int fat_numb;
	int root_directory_offset_sector;
	int data_area_offset;



char *append(const char *s, char c) {
    int len = strlen(s);
    char buf[len+2];
    strcpy(buf, s);
    buf[len] = c;
    buf[len + 1] = 0;
    return strdup(buf);
}

char* getName(int entry, uint8_t* directory, char* name){
	int i = 0;
	char* result = name;


	if ((int) directory[entry] == 5){ //as a special kludge, the first byte 0x05 means 0xe5
		result = append(result, (uint8_t) 229);
		i = 1; //read the next byte
	}

	for (i; i < 8; i++){
		if ((int) directory[i+entry] == 32){	//0x20 = 32 (space)
			break;
		}
		result = append(result, directory[i + entry]);	
						
	}
	return result;
}

char* getExt(int entry, uint8_t* directory, char* name){
	char* result = append(name, '.');
	int i;
	for (i = 8; i < 11; i++){
		result = append(result, directory[i + entry]); //adding extension to the name
	}
	return result;
				
}

void printAllClusters(int clusterNumber){ //excluding the first clusterNumber
	int nextClusterNumber = clusterNumber;
	while (isLastCluster(nextClusterNumber) == 0){
		printf("%d ", nextClusterNumber);
		nextClusterNumber = nextCluster(nextClusterNumber);
	}
}

int isLastCluster(int clusterNumber){
	int j;
	for (j = 0; j < 8; j++){
		if ( clusterNumber == lastCluster[j]){
			return 1;
		}
	}
	return 0;
}



int nextCluster(int clusterNumber){
		
	int nextClusterNumber;
	
	if (clusterNumber % 2 ==0){  // even clusterNumb
		return nextClusterNumber = FAT[clusterNumber /2 *3] | ((FAT[(clusterNumber /2 *3)+1] & 0x0F) << 8);
	}
	else{ // odd clusterNumb
		return nextClusterNumber = ((FAT[(clusterNumber /2 *3) +1 ] & 0xF0) >> 4) | (FAT[(clusterNumber /2 *3)+ 2] << 4);
	}

}



void printPart1 (char *filename) {// initialzes values of bootsector
	myfile = fopen(filename, "rb");

	int numb_items_read = fread((void*)boot_array, sizeof(uint8_t), 512, myfile);
	printf("Number of items read: %d \n", numb_items_read);
	

	sector_size = boot_array[11] | (boot_array[12] << 8);
	cluster_size = boot_array[13];
	root_directory_size = boot_array[17] | (boot_array[18] << 8);
	sectors_per_fat = boot_array[22] | (boot_array[23] << 8);
	reserved_sector = boot_array[14] | (boot_array[15] << 8);	
	hidden_sector = boot_array[28] | (boot_array[29] << 8);
	fat_offset_sector = reserved_sector;
	fat_numb = boot_array[16];
	root_directory_offset_sector = reserved_sector + sectors_per_fat * fat_numb;
	data_area_offset = root_directory_offset_sector + root_directory_size * 32/ sector_size;


	printf("Sector size: %d\n", sector_size);

	printf("Cluster size in sectors: %d\n", cluster_size); 

	printf("Root directory size (nb of entries): %d\n", root_directory_size); 

	printf("Sectors per fat: %d\n", sectors_per_fat );

	printf("Reserved sectors: %d\n", reserved_sector);

	printf("Hidden sectors: %d\n", hidden_sector);

	printf("Fat offset in sectors: %d\n", fat_offset_sector); 

	printf("Root directory offset in sectors: %d\n", root_directory_offset_sector);

	printf("First usable offset in sectors: %d\n \n", data_area_offset); 

}

void readSubDir(int clusterNumber, char* currentName){

	int offset = data_area_offset * sector_size + (clusterNumber - 2) * sector_size * cluster_size;
	fseek(myfile, offset, SEEK_SET);
	uint8_t subDirectory [sector_size * cluster_size];	
	fread((void*)subDirectory, sector_size, cluster_size, myfile);


	int entry = 0;
	int counter = 0; 

	
	while (subDirectory[entry] != 0 && (entry <= sector_size * cluster_size - 32)) // checking the boundary so that the function does not read beyond the array
	{
		char *name = currentName;
		
		if ((int) subDirectory[entry] != 229 && subDirectory[entry] != 32){  // 0xe5 = 229 (delete), 0x20 = 32 (space)

			int starting_cluster = subDirectory[entry + 26] | (subDirectory[entry + 27] << 8);
			int size = subDirectory[entry + 28] | (subDirectory [entry+29] << 8) | (subDirectory [entry+30] << 16) | (subDirectory [entry+31] << 24);
			
			name = append(name, '\\');		
			name = getName(entry, subDirectory, name);						
			

			if (size != 0){
				name = getExt(entry, subDirectory, name);
				printf("%s\n", name); 
				printf("Size: %d\n", size);

			}

			else {
				printf("%s\n", name); 
				printf("This file is a directory.\n");

			}

			printf("Clusters: %d ", starting_cluster ); 
			int nextClusterNumber = nextCluster(starting_cluster);
			printAllClusters(nextClusterNumber);
			printf("\n");


			
			if (counter >= 2 && size == 0){ // counter is used to avoid infinite loop when dealing with . and .. directories
				printf("\n");
				
				readSubDir(starting_cluster, name); // if directory, then repeat
				while (isLastCluster(nextClusterNumber) == 0){//since the subdirectory size can be anything, need to check if it uses more than one cluster
					readSubDir(nextClusterNumber, name);
					nextClusterNumber = nextCluster(nextClusterNumber);

				}	
			}
			
			
		}
		
		entry = entry + 32;
		counter = counter + 1;
		printf("\n");
		

	}
	
}



void printPart2 (){//the root directory
	fseek(myfile, root_directory_offset_sector * sector_size, SEEK_SET);	
	uint8_t directory_array[root_directory_size * 32];
 	fread((void*)directory_array, sizeof(uint8_t) * 32, root_directory_size, myfile);

	
	int entry = 0;

	while (directory_array[entry] != 0 && entry <= (root_directory_size * 32 -32))// checking the boundary so that the function does not read beyond the array
	{	
		char *name = "Filename: \\";

		if ((int) directory_array[entry] != 229 && directory_array[entry] != 32){  // 0xe5 = 229 (delete), 0x20 = 32 (space)

			int starting_cluster = directory_array[entry + 26] | (directory_array[entry + 27] << 8);
			int size = directory_array[entry + 28] | (directory_array [entry+29] << 8) | (directory_array [entry+30] << 16) | (directory_array [entry+31] << 24);
			
				
			name = getName(entry, directory_array, name);						



			if (size != 0){
				name = getExt(entry, directory_array, name);
				printf("%s\n", name); 
				printf("Size: %d\n", size);

			}

			else {
				printf("%s\n", name); 
				printf("This file is a directory.\n");

			}
			
			printf("Clusters: %d ", starting_cluster ); 			
			int nextClusterNumber = nextCluster(starting_cluster);
			printAllClusters(nextClusterNumber);
			printf("\n");

			
			if (size == 0){ //look for subdirectories (no need for counter as root directory entry does not have . and ..)

							printf("\n");	

				readSubDir(starting_cluster, name);

				while (isLastCluster(nextClusterNumber)==0){//since the subdirectory size can be anything, need to check if it uses more than one cluster
					readSubDir(nextClusterNumber, name);
					nextClusterNumber = nextCluster(nextClusterNumber);

				}

			}

			
		}
		
		entry = entry + 32;
		printf("\n");
	}
}




int main(int argc, char* argv[]){

	char *fileName = argv[1];
	printf("Filename is: %s \n", fileName);

	printPart1(fileName); // reads the boot sector with file name of "filename"
	

	//FAT initialization
	fseek(myfile, fat_offset_sector * sector_size, SEEK_SET);	
	uint8_t FAT12[sectors_per_fat * sector_size]; // FAT12
	fread((void*)FAT12, sector_size, sectors_per_fat, myfile);

	FAT = FAT12;

	printPart2();

}