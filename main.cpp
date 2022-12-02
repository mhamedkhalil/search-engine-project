#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<string>
#include<unordered_map>
#include<algorithm>
using namespace std;
unordered_map <string,double> pageRank;
unordered_map <string,int> Impressions;
unordered_map <string,int> Clicks;
unordered_map <string,vector<string>> Keywords;
unordered_map <string, double> CTR;
vector<pair <string,double>> search_results;
unordered_map <string,double> Score;
void search();
//first of all we need to initialize a Graph class to represent the links between websites
class Graph
{
    public:
    int V;//number of vertices (number of websites)
    // I will be using the unordered_map data structure to represent and visualize the links
    // The keys will be the website names and the value is a vector of websites linked to the key
    unordered_map<string,vector<string>> links;
    Graph()
    {
        V=0;
    }

    Graph(int size)
    {
        V=size;
    }
    void addEdge(string source, string destination)
    {
        links[source].push_back(destination);
    }
    //Initialize a function to print the graph
    void print()
    {
        for(auto i : links)
        {
            cout << i.first << "--> ";
            for(int j=0; j<i.second.size(); j++)
            {
                cout << i.second[j] << " / ";
            }
            cout << endl;
        }
    }
};

//alphabetical function is going to be used to sort keywords in alphabetical order (assending order)
bool alphabetical(string a, string b)
{ return a<b; }
//sortScore function is used to sort the search results according to their scores in descending order
bool sortScore(const pair<string,double> &a, const pair<string,double> &b)
{ return a.second>b.second; }

//get_websites is just a helping function that return the vertices to create the graph
vector<string> get_websites()
{
    vector<string> websites;
    string temp1, temp2, line;
    fstream webGraph;
    webGraph.open("web_pages.csv");
    if(!webGraph.good())
        cout << "Error! Could not open file" << endl;
    while(!webGraph.eof())
    {
        getline(webGraph,line);
        temp1=line.substr(0,line.find(","));
        temp2=line.substr(line.find(",")+1);
        websites.push_back(temp1);
        websites.push_back(temp2);
    }
    webGraph.close();
    return websites;
}

//a function to create our graph
Graph createGraph()
{
    int counter=0;
    vector<string> websites=get_websites();
    //the counter here is just the number of vertices
    for(int i=0; i<websites.size(); i+=2)
        counter ++;
    Graph g(counter);
    for(int i=0; i<websites.size(); i+=2)
         g.addEdge(websites[i],websites[i+1]);
    //g.print();
    return g;
}

//compute rank is a function that initialize websites rank that will be used in score calculation
void computeRank(Graph g)
{
    //the iterator will be used to keep iterating till we have previous=current
    bool iterator=true;
    unordered_map<string,double> previous;
    unordered_map<string,double> current;
    for(auto i : g.links)
        previous[i.first]=1.0/g.V;
    while(iterator)
    {
        for(auto i : g.links)
        {
            double temp=0;
            for(auto j : g.links)
            {
                for(int k=0; k<j.second.size(); k++)
                {
                    if(i.first == j.second[k])
                        temp+=(double)previous[i.first]/j.second.size();
                }
            }
            current[i.first]=temp;
        }

        for(auto i : current)
        {
            if(previous[i.first]==current[i.first])
                iterator = false;
            else previous[i.first]=current[i.first];
        }
    }
    
    for(auto i : g.links)
        pageRank[i.first]=current[i.first];
}
//This is a function to initialize the impressions that we already have
//It stores them in a global unordered_map so that they can be used by any function
void initialImpressions()
{
    //We will read the file and devide it into keys and values
    string temp1, temp2, line;
    fstream impression;
    impression.open("impressions_number.csv");
    if(!impression.good())
        cout << "Error! Could not open file" << endl;
    while(!impression.eof())
    {
        getline(impression,line);
        temp1=line.substr(0,line.find(","));
        temp2=line.substr(line.find(",")+1);
        Impressions[temp1]=stoi(temp2);
    }
    impression.close();
}
//This function has the same purpose of initialImpressions() but for clicks
//The same logic is applied here
void initialClicks()
{
    string temp1, temp2, line;
    fstream click;
    click.open("clicks.csv");
    if(!click.good())
        cout << "Error! Could not open file" << endl;
    while(!click.eof())
    {
        getline(click,line);
        temp1= line.substr(0,line.find(","));
        temp2= line.substr(line.find(",")+1);
        Clicks[temp1]=stoi(temp2);
    }
}
//update_impressions will be called each time we get a search result
//it updates the value of impressions in the global unordered_map and is the file as well
void update_impressions()
{
    for(auto i : search_results)
        Impressions[i.first]++;
    fstream update;
    update.open("impressions_number.csv");
    if(!update.good())
        cout << "Error! Could not open file" << endl;
    //the j is used only to avoid putting an extra "\n" at the end of the file 
    int j = 0;
    for(auto i : Impressions)
    {
        update << i.first << "," << i.second;
        if(j != Impressions.size()-1) update << "\n";
        j++;
    }
    update.close();
}
//the function keywords() is an initialization function 
//it devides the strings in the file "keywords.csv" into keys and value of vector of strings
//everything will be then stored in the global unordered_map Keywords
void keywords()
{
    fstream keyword;
    string temp1, temp2, line, temp;
    keyword.open("keywords.csv");
    if(!keyword.good())
        cout << "Error! Could not open file" << endl;
    while(!keyword.eof())
    {
        getline(keyword,line);
        vector<string> temp_sort;
        temp1=line.substr(0,line.find(","));
        temp2=line.substr(line.find(",")+1);
        stringstream s(temp2);
        while(!s.eof())
        {
            getline(s,temp,',');
            temp_sort.push_back(temp);
        }
        sort(temp_sort.begin(),temp_sort.end(),alphabetical);
        for(auto i : temp_sort)
            Keywords[temp1].push_back(i);
    }
}
//examineQuery function is mainly used to delete the extras in the query such as quotation, "and","or"
//it is also used to tell the main search() function which type of searching the user is seeking
//it breaks the users' query into keywords and return them to the search() function as a vector of strings
vector<string> examineQuery(string query, bool &And, bool &Or)
{
    vector<string> keywords;
    string temp;
    if(query[0]=='"')
    {
        //"removing double quotes from string" algorithm from codespeedy.com
        query.erase(remove(query.begin(),query.end(),'\"'),query.end());
        keywords.push_back(query);
    }
    else
    {
        if(query.find("and") != string :: npos)
        {
            And=true;
            query.erase(query.find("and"),4);
        }
        else if(query.find("or") != string :: npos)
        {
            Or=true;
            query.erase(query.find("or"),3);
        }
        else
            Or=true;

        stringstream s(query);
        while(s >> temp)
            keywords.push_back(temp);
    }
    return keywords;
}
//calculateScore() function is called while initializing the code and after every search operation
//the score calculated by this function uses data from different global data structure such as CTR and PR
//the score is used to sort the websites in the search_results in descending order
void calculateScore()
{
    double score;
    for(auto i : pageRank)
    {
        score= 0.4*i.second + ((1-(0.1*Impressions[i.first])/(1+0.1*Impressions[i.first]))*i.second + ((0.1*Impressions[i.first])/(1+0.1*Impressions[i.first]))*CTR[i.first])* 0.6;
        Score[i.first]=score;
    }
}
//calculateCTR() is also used to initialize CTR values to each website 
//it is used also to update these values before each search opertion
void calculateCTR()
{
    double temp;
    for(auto i : Impressions)
    {
        temp = (double)Clicks[i.first]/i.second;
        CTR[i.first]= temp*100;
    }
}

//updateClicks() function is used after each time the user chooses a website from the results
//the number of clicks will be updated in Clicks unordered_map and in the file as well
void updateClicks()
{
    fstream update;
    update.open("clicks.csv");
    if(!update.good())
        cout << "Error! Could not open file" << endl;
    
    int j = 0;
    for(auto i : Clicks)
    {
        update << i.first << "," << i.second;
        if(j != Impressions.size()-1) update << "\n";
        j++;
    }
    update.close();
}

//displayResults() is a function to display the found webpages after sorting 
//it is a user-friendly function that can let the user navigate between other functions
void displayResults()
{
    //it first displays the results of the initial query
    cout << "Your query results: " << endl;
    int choice;
    //if no results found, the user has the option to retry with another query or exit
    if(search_results.size()==0)
    { 
        cout << "No result was found\n";
        cout << "1-Try another query\n2-Exit\n";
        cout << "-------------------\n";
        cout << "Your choice: ";
        cin  >> choice;
        if(choice==1) {system("CLS");search();}
        if(choice==2) {exit(0);}
    }
    else
    {
        cout << search_results.size() << " website(s) found:\n";
        int counter=1;
        for(auto i : search_results)
        {
            cout << counter << "- " << i.first << endl;
            counter++;
        }
        cout << "----------------------------------\n\n";
        //after displaying the results, the user will have the option to choose a website from the results
        //or they can start a whole new search or exit the program
        do
        {
            cout << "Would you like to:\n1-Explore a website\n2-Start a new search\n3-Exit\n";
            cout << "----------------------------------\nYour choice: ";
            cin  >> choice;
            if(choice == 1)
            {
                //choosing a website is counted as a click
                int click;
                system("CLS");
                counter =1;
                for(auto i : search_results)
                {
                    cout << counter << "- " << i.first << endl;
                    counter++;
                }
                cout << "----------------------------------\n\n";
                bool condition=0;
                do
                {
                    cout << "Enter the website number: ";
                    cin  >> click;
                    if(click>search_results.size() || click == 0 || click < 0)
                    {
                        cout << "Unidentified website\n\n";
                    }
                    else
                    {
                        condition=1;
                        Clicks[search_results[click-1].first]++;
                        updateClicks();
                        cout << "You are now viewing " << search_results[click-1].first << endl;
                        for(int i=0; i<Keywords[search_results[click-1].first].size(); i++)
                            cout << Keywords[search_results[click-1].first][i] << " ";
                        cout << endl;
                        system("PAUSE");
                        system("CLS");
                    }
                    //even after opening a website, the user will have the option to research another query without closing the program
                }while(condition == 0);
            }
            else if(choice == 2)
            {
                system("CLS");
                search();
            }
            else if(choice == 3) { exit(0); }
            else {cout << "Unrecognized command\n"; system("PAUSE"); system("CLS");}
        }while(choice != 1 || choice != 2 || choice != 3);
    }
}
//the search() function is the core of the program 
void search()
{
    //it initialize CTR and score values for each website everytime it is called
    calculateCTR();
    calculateScore();
    //it clears the already saved results in case another searching operation was initiated before closing the program
    search_results.clear();
    bool And=0,Or=0;
    string s;
    system("CLS");
    cout << "Enter your search query between quotation if you want the results to contain the query exactly as it is\n";
    cout << "Using 'and' in your query will give you the results that contain all the words you entered\n";
    cout << "Using 'or' in your query or not including quotation, 'and' or 'or' will give you the results that contain at least one keywords from your query\n";
    cout << "=============================================================================================================================\n";
    cout << "Enter your query: ";
    cin.ignore();
    getline(cin,s);
    vector<string> key=examineQuery(s,And,Or);
    //each type of queries has its own searching way
    if(And == 0 && Or == 0)
    {
        //the counter here is used to iterate exactly the number of keywords found between quotation
        int counter=0;
        string temp;
        stringstream s_temp(key[0]);
        vector<string> temp_strings;
        while(s_temp >> temp)
        {
            counter++;
            temp_strings.push_back(temp);
        }
        for(auto i : Keywords)
        {
            if(binary_search(i.second.begin(),i.second.end()-1,temp_strings[0]))
            {
                if(temp_strings.size() == 1)
                    search_results.push_back(make_pair(i.first,Score[i.first]));
                else
                {
                    auto it = find(i.second.begin(), i.second.end(), temp_strings[0]);
                    int n=it-i.second.begin();
                    for(int j=1; j<temp_strings.size(); j++)
                    {
                        bool test=0;
                        if(i.second[n+j] == temp_strings[j])
                            test=1;
                        if(j+1 == temp_strings.size() && test == 1)
                            search_results.push_back(make_pair(i.first,Score[i.first]));
                    }
                }
            }
        }
    }
    else if(And)
    {
        //counter here is used to determine wheter we found every keywords in the query or not
        int count1=0;
        for(auto i : key)
        {
            for(auto j :Keywords)
            {
                if(binary_search(j.second.begin(), j.second.end() ,i))
                    count1++;
                if(count1 == key.size())
                {
                    search_results.push_back(make_pair(j.first,Score[j.first]));
                    count1=0;
                }
            }
        }
    }
    else
    {
        //counter here is to determine whether we found at least one keyword from the query or not
        int count=0;
        for(auto i : key)
        {
            for(auto j :Keywords)
            {
                if(binary_search(j.second.begin(), j.second.end() ,i))
                    count++;
                if(search_results.size() != 0)
                {
                    if(count > 0 && search_results[search_results.size()-1].first != j.first)
                    {
                        search_results.push_back(make_pair(j.first,Score[j.first]));
                        count=0;
                    }
                }
                else
                {
                    if(count > 0)
                    {
                        search_results.push_back(make_pair(j.first,Score[j.first]));
                        count=0; 
                    }
                }
            }
        }
    }
    //after fillinf the search_results vector we need to sort it by scores and update the impression then display the results
    sort(search_results.begin(),search_results.end(),sortScore);
    update_impressions();
    displayResults();
}
//specificWeb() function is only called when the user chooses to search by website titles and not by keywords
//if the website title written by the user was found, it will be counted as an extra click and impression for this website
void specificWeb()
{
    string website;
    system("CLS");
    cout << "Enter the website you want to explore: ";
    cin  >> website;
    cin.ignore();
    if(Keywords.find(website) != Keywords.end())
    {
        system("CLS");
        cout << "You are now viewing " << website << endl;
        Clicks[website]++; Impressions[website]++;
        updateClicks(); update_impressions();
        for(int i=0; i<Keywords[website].size(); i++)
            cout << Keywords[website][i] << " ";
        cout << endl;
    }
    else
        cout << "Website not found\n";
}
//initializeCode() is going to be called once when launching the first search operation 
//it gives initial values to some data that are going to be used in the search operation
void initializeCode()
{
    Graph g=createGraph();
    keywords();
    initialImpressions();
    initialClicks();
    computeRank(g);
}

int main()
{
    int choice;
    cout << "Welcome!\n";
    do
    {
        cout << "What would you like to do?\n";
        cout << "1-Start a new search by keywords\n2-Explore a specific website\n3-Exit\n";
        cout << "---------------------------\n";
        cout << "Enter your choice: ";
        cin  >> choice;
        if(choice == 1)
        {
            initializeCode();
            search();
        }
        if(choice == 3){exit(0);}
        if(choice == 2)
        {
            initializeCode();
            specificWeb();
        }
        else{cout << "Unrecognized command\n";}
    }while(choice != 1 || choice != 2 || choice != 3);
    return 0;
}