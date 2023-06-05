#include <functional>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

const int ch_MAX = 35;
	
struct User
{
	std::string first_name;
	std::string last_name;
    	bool operator==(const User&) const = default; // since C++20
};

struct MyHash
{
    	std::size_t operator()(User const& s) const noexcept
    	{
        	std::size_t h1 = std::hash<std::string>{}(s.first_name);
        	std::size_t h2 = std::hash<std::string>{}(s.last_name);
        	return h1 ^ (h2 << 1); // or use boost::hash_combine
    	}
};

// custom specialization of std::hash can be injected in namespace std
template<>
struct std::hash<User>
{
    	std::size_t operator()(User const& s) const noexcept
    	{
        	std::size_t h1 = std::hash<std::string>{}(s.first_name);
        	std::size_t h2 = std::hash<std::string>{}(s.last_name);
        	return h1 ^ (h2 << 1); // or use boost::hash_combine
    	}
};

int count_alphas(const std::string& s)
{
        return std::count_if(s.begin(), s.end(), [](unsigned char c){
                return std::isalpha(c);
        });
}

std::string random_str(int ch)
{
        char alpha[ch_MAX] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                          'h', 'i', 'j', 'k', 'l', 'm', 'n',
                          'o', 'p', 'q', 'r', 's', 't', 'u',
                          'v', 'w', 'x', 'y', 'z', '!', '%',
                          '{', '}', '@', '?', '&', '^', '=' };
        std::string result = "";
        for (int i = 0; i<ch; i++)
                result = result + alpha[rand() % ch_MAX];

    	return result;
}

int main(int argc, char* argv[])
{
    	std::string fname;
    	std::string lname;
    	bool cargs = false;
    	bool canha = true;
    	if (argc == 3)
    	{
		cargs = true;
		for (int i = 0; i < argc; i++)
		{
			if (i == 1)
				fname = argv[i];
                    	if (i == 2)
				lname = argv[i];
		}
	}
	std::string str = "almond_api_token_generator";
	std::cout << std::quoted(str) << std::endl;
	std::cout << "<Creates an user hash for Almond API>" << std::endl << std::endl;

	if (!cargs)
    	{
		std::cout << "Enter your first name: " << std::endl;
		std::cin >> fname;
            	std::cout << "Enter your last name: " << std::endl;
            	std::cin >> lname;
    	}

	if (fname.length() < 2) {
		std::cout << "Your first name ('" << fname << "') seems odd." << std::endl;
            	canha = false;
    	}
    	else {
		int c = count_alphas(fname);
		if (c != fname.length())
            	{
			std::cout << "There seem to be some odd characters in your firstname." << std::endl;
                    	canha = false;
            	}
    	}

    	if (lname.length() < 2) {
		std::cout << "Your family  name ('" << lname << "') seems odd." << std::endl;
            	canha = false;
    	}
    	else {
		int c = count_alphas(lname);
		if (c != lname.length())
            	{
			std::cout << "There seem to be some odd characters in your lastname." << std::endl;
                    	canha = false;
            	}
    	}
    	if (canha)
    	{
        	User obj = { fname, lname };

		long unsigned int h = MyHash{}(obj);
        	srand(time(NULL));
        	std::string token = random_str(5) + std::to_string(h) + random_str(5);
        	std::cout << "Your hash is: " << token << std::endl;
        	std::string ws = obj.first_name + " " + obj.last_name + "\t" + token + "\n"; // + MyHash{}(obj);
        	std::ofstream hFile;
        	hFile.open("/etc/almond/tokens", std::ios_base::app);
        	hFile << ws;
        	hFile.close();
        	std::cout << "Token added to Almond configuration." << std::endl;

    	}
    	else {
		std::cout << "Failed to create hash. The supplied name could not be parsed." << std::endl;
    	}
}

