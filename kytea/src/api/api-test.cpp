#include <iostream>

// a file including the main program
#include <kytea/kytea.h>
// a file including sentence, word, and pronunciation objects
#include <kytea/kytea-struct.h>
// a file to include the StringUtil object
#include <kytea/string-util.h>

using namespace std;
using namespace kytea;

int main(int argc, char** argv) {

    // Create an instance of the Kytea program
    Kytea kytea;
    
    // Load a KyTea model from a model file
    //  this can be a binary or text model in any character encoding,
    //  it will be detected automatically
    const char *path = argc > 1 ? argv[1] : "../../data/model.bin";
    kytea.readModel(path);

    // Get the string utility class. This allows you to convert from
    //  the appropriate string encoding to Kytea's internal format
    StringUtil* util = kytea.getStringUtil(); 

    // Get the configuration class, this allows you to read or set the
    //  configuration for the analysis
    KyteaConfig* config = kytea.getConfig();

    // Map a plain text string to a KyteaString, and create a sentence object
    const char* s = argc > 2 ? argv[2] : "これはテストです。";
    KyteaString surface_string = util->mapString(s);
    KyteaSentence sentence(surface_string, util->normalize(surface_string));

    // Find the word boundaries
    kytea.calculateWS(sentence);
    // Find the pronunciations for each tag level
    for(int i = 0; i < config->getNumTags(); i++)
        kytea.calculateTags(sentence,i);


    // For each word in the sentence
    const KyteaSentence::Words & words =  sentence.words;
    for(int i = 0; i < (int)words.size(); i++) {
        // Print the word
    	cout << words[i].surface.length() << ":" << util->showString(words[i].surface).size();
    	cout << "|";
        cout << util->showString(words[i].surface);
        // For each tag level
        for(int j = 0; j < (int)words[i].tags.size(); j++) {
            cout << "\t";
            // Print each of its tags
            for(int k = 0; k < (int)words[i].tags[j].size(); k++) {
                cout << " " << util->showString(words[i].tags[j][k].first) << 
                        "/" << words[i].tags[j][k].second;
            }
        }
        cout << endl;
    }
    cout << endl;

}
