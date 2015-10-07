/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include <iostream>
#include <stdexcept>

#include "vdb-search.hpp"

using namespace std;

typedef vector < string > Runs;

void 
DoSearch ( const string& p_query, const Runs& p_runs, const string& p_alg  )
{
    VdbSearch s;
    s . SetQuery ( p_query );
    
    if ( ! p_alg . empty() && ! s . SetAlgorithm ( p_alg ) )
    {
        throw invalid_argument ( string ( "Bad argument for --algorithm : '" ) + p_alg + "'" );
    }
    
    for ( Runs :: const_iterator i = p_runs . begin (); i != p_runs . end (); ++ i )
    {
        s . AddAccession ( *i );
    }
    
    string acc;
    string fragId;
    while ( s . NextMatch ( acc, fragId ) )
    {
        cout << fragId << endl;
    }
}

static void handle_help ( const char * appName )
{
    string fileName = appName;
    string::size_type filePos = fileName . rfind ( '/' );
    if ( filePos != string::npos)
    {
        fileName = fileName . substr ( filePos + 1 );
    }

    cout << endl 
        << "Usage:" << endl 
        << "  " << fileName << " [Options] query accession" << endl 
        << endl 
        << "Summary:" << endl
        << "  Searches all reads in the accession and prints Ids of all the fragments that contain a match." << endl
        << endl 
        << "Example:" << endl
        << "  sra-search ACGT SRR000001" << endl
        << endl 
        << "Options:" << endl 
        << "  -h|--help                 Output brief explanation of the program." << endl
        << "  -a|--algorithm <alg>      Search algorithm, one of:" << endl;
        
    const SraSearch :: SupportedAlgorithms algs = VdbSearch () . GetSupportedAlgorithms ();
    for ( SraSearch :: SupportedAlgorithms :: const_iterator i = algs . begin (); i != algs . end (); ++i )
    {
        cout << "      " << *i;
        if ( i - algs . begin () == VdbSearch :: Default )
        {
            cout << " (default)";
        }
        cout << endl;
    }
    
    cout << endl;
}

int
main( int argc, char *argv [] )
{
    int rc = -1;
    
    try
    {
        string query;
        Runs runs;
        string alg;
        
        unsigned int i = 1;
        while ( i < argc )
        {
            string arg = argv [ i ];
            if ( arg [ 0 ] != '-' )
            {   
                if ( query . empty () )
                {
                    query = arg;
                }
                else
                {   // an input run
                    runs . push_back ( arg );
                }
            }
            else if ( arg == "-h" || arg == "--help" )
            {
                handle_help ( argv [ 0 ]  );
                return 0;
            }
            else if ( arg == "-a" || arg == "--algorithm" )
            {
                ++i;
                if ( i >= argc )
                {
                    throw invalid_argument ( "Missing argument for --algorithm" );
                }
                alg = argv [ i ];
            }
            else
            {
                throw invalid_argument ( "Invalid argument" );
            }
            
            ++i;
        }
        
        if ( query . empty () || runs . size () == 0 )
        {
            throw invalid_argument ( "Missing arguments" );
        }

        DoSearch ( query, runs, alg );

        rc = 0;
    }
    catch ( invalid_argument & x )
    {
        cerr << "ERROR: " << x . what () << endl;
        handle_help ( argv [ 0 ] );
        rc = 1;
    }
    catch ( exception & x )
    {
        cerr << "ERROR: " << argv [ 0 ] << ": " << x . what () << endl;
        rc = 2;
    }
    catch ( ... )
    {
        cerr << "ERROR: "<< argv [ 0 ] << ": unknown" << endl;
        rc = 3;
    }

    return rc;
}
