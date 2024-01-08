struct enod
{
    int id;
    int isFile;
    int depth; // this could be determined in multiple moments
    char name[128]; // in fetch detected
    char localPath[1024]; // this is the locatuon in the download folder of the file
    char url[1024]; // this is the location for the fetch
    char baseUrl[1024]; // so that we can deduce the path of the actual file
    char fileContent[1000 * 1024]; // in fetch
    char header[256];
};
void fileFetcher(struct enod* ENOD);