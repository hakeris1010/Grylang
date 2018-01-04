#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <sstream>
#include <string>

const size_t SAMPLE_SIZE = 50000;
const size_t ITERATIONS = 10000;
const size_t BUFFSIZE = 2048;

const bool PRINT_SAMPLE = false;

template<typename Callable, typename... Args>
void functionExecTime( Callable func, Args&&... args ){
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    func( std::forward<Args>(args)... );

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "f() took " << time_span.count() << " ms\n";
}

struct StreamStats{
    size_t lineCount = 0;
    size_t posInLine = 0;

    StreamStats(){}
    StreamStats( size_t lnc, size_t posc )
        : lineCount( lnc ), posInLine( posc ) 
    {}

    void print( std::ostream& out ) const {
        out<<"StreamStats:\n Lines: "<< lineCount <<"\n posInLine: "<< posInLine<<"\n";
    }
};

inline std::ostream& operator<<( std::ostream& os, const StreamStats& sts ){
    sts.print( os );
    return os;
}

StreamStats readCharByChar(std::istream& is){
	size_t posInLine = 0;
	size_t lineCount = 0;

	std::string buff;
	buff.reserve(256);

    //std::cout<<"Reading CHar by Char\n";
	while( 1 ){
		char c = is.get();

        if( is.eof() )
            break;

		if( c == '\n' ){
            //std::cout<<"["<< is.tellg() <<"]Found Endl!\n";
			posInLine = 0;
			lineCount++;
		}
        else
            posInLine++;

        buff.push_back( c );
	}

    return StreamStats( lineCount, posInLine );
}

StreamStats readWholeStream(std::istream& is, size_t buffSize){
    size_t posInLine = 0;
	size_t lineCount = 0;

	std::string buff( buffSize, '\0' );

    while( !is.eof() ){
        is.read( &buff[0], buff.size() );
        size_t readct = is.gcount();

        for( size_t i = 0; i < readct; i++ ){
            if( buff[i] == '\n' ){
                posInLine = 0;
                lineCount++;
            }
            else
                posInLine++;
        }
    }

    return StreamStats( lineCount, posInLine );
}

void cbcXtimes( std::istream& is, size_t times ){
    StreamStats stats;

    for ( size_t i = 0; i < times; i++ ){
        //std::cout<< "Loop "<<i<<"\n";

        stats = readCharByChar( is );

        is.clear();
        is.seekg(0, is.beg);
    }

    std::cout <<"CBC Results: "<< stats <<"\n";
}

void buffXtimes( std::istream& is, size_t times, size_t buffSize ){
    StreamStats stats;

    for ( size_t i = 0; i < times; i++ ){
        stats = readWholeStream( is, buffSize );

        is.clear();
        is.seekg(0, is.beg); 
    }

    std::cout <<"BUFF Results: "<< stats <<"\n";
}

void generateSample( std::string& str, size_t sampleSize, size_t maxLineSize = 80 ){
    size_t nextLinePos = 0 + (rand() % maxLineSize);

    for( size_t i = 0; i < sampleSize; i++ ){
        char cc;
        if( i >= nextLinePos ){
            nextLinePos = i + (rand() % maxLineSize);
            cc = '\n';

            //std::cout<<" NextLinePos: "<< nextLinePos <<" (i + "<< nextLinePos - i <<")\n";
        }
        else
            cc = char(32) + rand() % 90;

        str.push_back( cc );
    }
}

int main(){
    srand( time(0) );

    std::string sample;
    sample.reserve( SAMPLE_SIZE );

    generateSample( sample, SAMPLE_SIZE );
    if( PRINT_SAMPLE)
        std::cout << sample <<"\n\n\n";

    std::istringstream sstr( sample, std::ios_base::in | std::ios_base::binary );

    std::cout<< "CharByChar:\n";
    functionExecTime( cbcXtimes, sstr, ITERATIONS );
 
    std::cout<< "\n\nbuffXtimes:\n";
    functionExecTime( buffXtimes, sstr, ITERATIONS, BUFFSIZE );
    
    return 0;
}

