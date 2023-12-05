struct enod
{
    int id;
    int isFile; // this is found in the moment of the fetch
    int depth; // this could be determined in multiple moments
    char name[128]; // in fetch detected
    char fileExtension[128]; // in fetch detected
    char localPath[1024]; // this is the locatuon in the download folder of the file
    char url[1024]; // this is the location for the fetch
    char fileContent[10 * 1024]; // in fetch
};
void fileFetcher(struct enod* ENOD);