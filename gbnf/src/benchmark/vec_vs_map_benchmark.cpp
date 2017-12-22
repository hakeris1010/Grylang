#include <gbnf.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>

const size_t arrSize = 3000000;
const size_t findIters = 10000000;

template<typename Callable, typename... Args>
void functionExecTime( Callable func, Args&&... args ){
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    func( std::forward<Args>(args)... );

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "f() took " << time_span.count() << " ms\n";
}

void generateTestData( size_t dataSamples, size_t step,
        auto& data, std::vector<size_t>& randVals ){
    // Fill stuff.
    for( size_t i=0; i < dataSamples; i += step ){
        data.insertRule( gbnf::GrammarRule( i, {
            gbnf::GrammarToken( gbnf::GrammarToken::ROOT_TOKEN, i, std::to_string(i), {} )
        } ) );

        data.insertTag( "tag_"+std::to_string( i ) );

        if( i/step < randVals.size() ){
            randVals[ i/step ] = (size_t)( ((float)rand()) * 
                ((double)dataSamples / (double)RAND_MAX)) % dataSamples;
        }
    } 
}

void testVector(){
    // Benchmark vector-type container.
    std::cout<<"\n------- Vector -------\n\nAllocating memory...\n" << std::flush;

    gbnf::GbnfData data;
    std::vector<size_t> randVals( arrSize );

    // Fill array
    generateTestData( arrSize*2, 2, data, randVals );

    std::cout<<"Search Rule for "<< findIters <<" iterations.\n" << std::flush;

    functionExecTime( [&](){
        for(size_t i=0; i < findIters; i++){
            //if( !(i%50000) )
            //    std::cout<<"["<< i <<"]: Search RULE at pos: "<< randVals[ i % arrSize ] <<"\n";

            data.getRule( randVals[ i % arrSize ] );
        }
    } );

    std::cout<<"\n--------------\nSearch Tag for "<< findIters <<" iterations.\n" << std::flush;
    functionExecTime( [&](){
        for(size_t i=0; i < findIters; i++){      
            //if( !(i%50000) )
            //    std::cout<<"["<< i <<"]: Search TAG at pos: "<< randVals[ i % arrSize ] <<"\n"; 
            data.getTag( randVals[ i % arrSize ] );
        }
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds(5000) );
}

void testMap(){
    // Benchmark Map-type container.
    std::cout<<"\n\n-------- Map --------\n\nAllocating memory...\n" << std::flush;

    gbnf::GbnfData_Map mData;
    std::vector<size_t> randVals( arrSize );

    // Fill array
    generateTestData( arrSize*2, 2, mData, randVals );

    std::cout<<"Search Rule for "<< findIters <<" iterations.\n" << std::flush;

    functionExecTime( [&](){
        for(size_t i=0; i < findIters; i++){
            //if( !(i%50000) )
            //    std::cout<<"["<< i <<"]: Search RULE at pos: "<< randVals[ i % arrSize ] <<"\n";
            mData.getRule( randVals[ i % arrSize ] );
        }
    } );

    std::cout<<"\n--------------\nSearch Tag for "<< findIters <<" iterations.\n" << std::flush;
    functionExecTime( [&](){
        for(size_t i=0; i < findIters; i++){      
            //if( !(i%50000) )
            //    std::cout<<"["<< i <<"]: Search TAG at pos: "<< randVals[ i % arrSize ] <<"\n"; 
            mData.getTag( randVals[ i % arrSize ] );
        }
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
}

int main(){
    srand( time(0) );
    std::cout<<"Benchmarking GBNF lookup performance. Arr lenght: "<< arrSize <<"\n";
    
    testVector();
    testMap();

    return 0;
}

