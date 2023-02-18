#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

const int BLOCK_SIZE = 4096;
using namespace std;

vector<pair<int, int>> all_block_sets;
unsigned int max_file_size;

vector<string> split(const string& s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

vector<pair<int, int>> rangeset(string src) {
    vector<string> src_set = split(src, ',');
    vector<int> num_set;
    for (auto item : src_set) {
        num_set.push_back(stoi(item));
    }
    if (num_set.size() != num_set[0] + 1) {
        cerr << "Error on parsing following data to rangeset:\n" << src << endl;
        exit(1);
    }
    vector<pair<int, int>> result;
    for (int i = 1; i < num_set.size(); i += 2) {
        result.push_back(make_pair(num_set[i], num_set[i + 1]));
    }
    return result;
}

pair<int, vector<pair<int, int>>> parse_transfer_list_file(string path) {
    ifstream trans_list(path);
    int version;
    string line;
    getline(trans_list, line);
    version = atoi(line.c_str()); // First line in transfer list is the version number
    int new_blocks;
    getline(trans_list, line);
    new_blocks = atoi(line.c_str()); // Second line in transfer list is the total number of blocks we expect to write
    if (version == 1) {
        cout << "Android 5.0 detected!\n" << endl;
    }
    else if (version == 2) {
        cout << "Android 5.1 detected!\n" << endl;
    }
    else if (version == 3) {
        cout << "Android 6.x detected!\n" << endl;
    }
    else if (version == 4) {
        cout << "Android 7.0+ detected!\n" << endl;
    }
    else {
        cout << "Unknown Android version!\n" << endl;
    }
    if (version >= 2) {
        trans_list.get(); // Third line is how many stash entries are needed simultaneously
        trans_list.get(); // Fourth line is the maximum number of blocks that will be stashed simultaneously
    }

    // Subsequent lines are all individual transfer commands
    while (getline(trans_list, line)) {
        vector<string> line_split = split(line, ' ');
        string cmd = line_split[0];
        if (cmd == "new") {
            vector<pair<int, int>> block_sets = rangeset(line_split[1]);
            all_block_sets.insert(all_block_sets.end(), block_sets.begin(), block_sets.end());
        }
    }
    trans_list.close();

    int max_pair = 0;
    for (auto block : all_block_sets) {
        max_pair = max(max_pair, block.second);
    }
    max_file_size = max_pair * BLOCK_SIZE; // Rezult file size

    return make_pair(max_file_size, all_block_sets);
}
// Create empty image with Correct size;
void initOutputFile(ofstream* output_file_obj, int BLOCK_SIZE) {
    long long position = max_file_size - 1;
    unsigned long offset = position % ULONG_MAX;
    int cycles = position / ULONG_MAX;

    // in the case of images greater than 4GB if its even possible
    if (cycles > 0) {
        output_file_obj->seekp(0, ios::beg);
        for (int i = 0; i < cycles; i++) {
            output_file_obj->seekp(ULONG_MAX, ios::cur);
        }
        output_file_obj->seekp(offset, ios::cur);
    }
    else output_file_obj->seekp(position);

    output_file_obj->put('\0');
    output_file_obj->flush();
    return;
}

int main(int argc, char* argv[]) {
    cout << "\nCredits to blackeangel at blackeangel@mail.ru (4PDA.ru) special for UKA tools" << endl;
    cout << "Re-written in C++ by me from xpirt - luxi78 - howellzhu work in python\n" << endl;

    if (argc != 4) {
        cout << "\nsdat2img - usage is: \n\n      sdat2img <transfer_list> <system_new_file> <system_img>\n\n";
        exit(1);
    }

    char* TRANSFER_LIST_FILE = argv[1];
    char* NEW_DATA_FILE = argv[2];
    char* OUTPUT_IMAGE_FILE = argv[3];

    ifstream transfer_list_file(TRANSFER_LIST_FILE);
    if (!transfer_list_file.is_open()) {
        cout << TRANSFER_LIST_FILE << " not found" << endl;
        exit(2);
    }
    transfer_list_file.close();

    parse_transfer_list_file(TRANSFER_LIST_FILE);

    ifstream new_data_file(NEW_DATA_FILE, ios::binary);
    if (!new_data_file.is_open()) {
        cout << NEW_DATA_FILE << " not found" << endl;
        exit(2);
    }
    ofstream output_img(OUTPUT_IMAGE_FILE, ios::binary);
    if (!output_img.is_open()) {
        cout << OUTPUT_IMAGE_FILE << " not found" << endl;
        exit(2);
    }

    /*vector<char> data(BLOCK_SIZE);
    for (pair<int, int> block : all_block_sets) {
        int begin = block.first;
        int end = block.second;
        int block_count = end - begin;
        //cout << "Copying " << block_count << " blocks into position " << begin << "..." << endl;
        output_img.seekp(begin * BLOCK_SIZE); // Position output file
        // Copy one block at a time

        while (block_count > 0) {
            new_data_file.read(data.data(), BLOCK_SIZE);
            output_img.write(data.data(), BLOCK_SIZE);
            //
            output_img.seekp(0, ios::end); // если курсор в начале файла, перемещаем курсор в конец файла.
            int sizef = output_img.tellp(); // функция выдаст конечное положнние курсора относительно начала файла в байтах.
            cout << "Size: " << sizef << ", block count: " << block_count <<", begin: " << begin << ", end: " << end << endl;
            //
            block_count -= 1;
        }

    }
    // Make file larger if necessary
    if (output_img.tellp() < max_file_size) {
        output_img.seekp(max_file_size);
    }

    output_img.close();
    new_data_file.close();
    cout << "Done!" << endl;

    return 0;
    */
    initOutputFile(&output_img, BLOCK_SIZE);

    uint8_t* data;
    bool quiet = false;
    for (pair<int, int> block : all_block_sets) {
        long begin = block.first;
        long end = block.second;
        long block_count = end - begin;
        long blocks = block_count * BLOCK_SIZE;
        long long position = begin * BLOCK_SIZE;
        unsigned long offset = position % ULONG_MAX;
        int cycles = position / ULONG_MAX;

        data = (uint8_t*)malloc(blocks);
        if (data == NULL) {
            cout << "Out of memory error" << endl;
            exit(-1);
        }
        new_data_file.read((char*)data, blocks);

        // in the case of images greater than 4GB if its even possible
        if (cycles > 0) {
            output_img.seekp(0, ios::beg);
            for (int i = 0; i < cycles; i++) {
                output_img.seekp(ULONG_MAX, ios::cur);
            }
            output_img.seekp(offset, ios::cur);
        }
        else output_img.seekp(position);

        if (!quiet) {
            cout << "Copying " << block_count << " blocks into position " << begin << " with " << blocks << " bytes" << endl;
        }

        output_img.write((char*)data, blocks);
        free(data);
    }

    output_img.close();
    new_data_file.close();

    cout << "\nDone!" << endl;
    return 0;
}
