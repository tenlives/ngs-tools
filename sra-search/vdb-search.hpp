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

#ifndef _hpp_vdb_search_
#define _hpp_vdb_search_

#include <string>
#include <vector>
#include <queue>
#include <stdexcept>

#include <ngs/ReadCollection.hpp>

class VdbSearch
{
public:
    // base class of a hierarchy implementing various search algorithms 
    class SearchBlock
    {   
    public:
        virtual ~SearchBlock () {}
        
        virtual bool FirstMatch ( const char* p_bases, size_t p_size ) = 0;
    };
    
public:
    static bool logResults;

public:
    // Search algorithms supported by this class
    typedef enum
    {
        FgrepDumb = 0, 
        FgrepBoyerMoore,
        FgrepAho,
        AgrepDP,
        AgrepWuManber,
        AgrepMyers,
        AgrepMyersUnltd,
        NucStrstr,
        SmithWaterman,
    } Algorithm;

    typedef std :: vector < std :: string >  SupportedAlgorithms;

    static SupportedAlgorithms GetSupportedAlgorithms (); // enum Algorithm values correspond to indexes in the container returned here
    
public:
    VdbSearch ( Algorithm, const std::string& query, bool isExpression ) throw ( std :: invalid_argument );
    VdbSearch ( const std :: string& algorithm, const std::string& query, bool isExpression ) throw ( std :: invalid_argument );
    
    ~VdbSearch ();
    
    Algorithm GetAlgorithm () const { return m_algorithm; }
    
    void AddAccession ( const std::string& ) throw ( ngs :: ErrorMsg );
    
    bool NextMatch ( std::string& accession, std::string& fragmentId ) throw ( ngs :: ErrorMsg );
    
private:
    // a VDB-agnostic iterator bound to an accession and an engine-side algorithm
    class MatchIterator 
    {
    public:
        MatchIterator ( SearchBlock*, const std::string& accession );
        ~MatchIterator ();
        
        bool NextMatch ( std::string& fragmentId );
        
        std::string AccessionName () const;
        
    private: 
        ngs::ReadCollection m_coll;
        ngs::ReadIterator   m_readIt;
        
        SearchBlock* m_searchBlock;  // owned here
    };
    
    bool SetAlgorithm ( const std :: string& p_algStr );
    
    static SearchBlock* SearchBlockFactory ( const std :: string& p_query, bool p_isExpression, Algorithm p_algorithm );

private:
    std::string         m_query;
    bool                m_isExpression; 
    Algorithm           m_algorithm;

    std::queue < MatchIterator* > m_searches;
};

#endif
