#include <gbnf/gbnf.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>

const size_t arrSize = 3000000;
const size_t findIters = 500000000;

template<typename Callable, typename... Args>
void functionExecTime( Callable func, Args&&... args ){
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    func( std::forward<Args>(args)... );

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "f() took " << time_span.count() << " ms\n";
}

int main(){
    srand( time(0) );

    std::cout<<"Benchmarking GBNF lookup performance. Arr lenght: "<< arrSize;
    std::cout<<"\n\nAllocating memory...\n" << std::flush;

    gbnf::GbnfData data;
    size_t randVals[ arrSize ];

    // Fill array
    for( size_t i=0; i < arrSize; i++ ){
        data.insertRule( gbnf::GrammarRule( i, {
            gbnf::GrammarToken( gbnf::GrammarToken::ROOT_TOKEN, i, std::to_string(i), {} )
        } ) );

        data.insertTag( "tag_"+std::to_string( i ) );

        randVals[i] = (size_t)(((float)rand()) * ((double)arrSize/(double)RAND_MAX)) % arrSize;
    }

    std::cout<<"\n--------------\nVector:\n\n";
    std::cout<<"Search Rule for "<< findIters <<" iterations.\n" << std::flush;

    functionExecTime( [&](){
        for(size_t i=0; i < findIters; i++){
            //std::cout<<"["<< i <<"]: Search rule\n";
            data.getRule( randVals[ i % arrSize ] );
        }
    } );

    std::cout<<"Search Tag for "<< findIters <<" iterations.\n" << std::flush;
    functionExecTime( [&](){
        for(size_t i=0; i < findIters; i++){      
            //std::cout<<"["<< i <<"]: Search tag\n";
            data.getTag( randVals[ i % arrSize ] );
        }
    } );

    return 0;
}

