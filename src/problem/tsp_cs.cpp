/*****************************************************************************
 *   Copyright (C) 2004-2014 The PaGMO development team,                     *
 *   Advanced Concepts Team (ACT), European Space Agency (ESA)               *
 *   http://apps.sourceforge.net/mediawiki/pagmo                             *
 *   http://apps.sourceforge.net/mediawiki/pagmo/index.php?title=Developers  *
 *   http://apps.sourceforge.net/mediawiki/pagmo/index.php?title=Credits     *
 *   act@esa.int                                                             *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#include "tsp_cs.h"
#include "../population.h"

namespace pagmo { namespace problem {

    /// Default constructor
    /**
     * This constructs a 3-cities symmetric problem (naive TSP_CS) 
     * with weight matrix [[0,1,1], [1,0,1], [1,1,0]], value vector [1,1,1]
     * maximum path length of 1 and RANDOMKEYS encoding
     */
    tsp_cs::tsp_cs() : base_tsp(3, 0, 0 , base_tsp::RANDOMKEYS), m_weights(), m_values(), m_max_path_length(1.0)
    {
        std::vector<double> dumb(3,0);
        m_weights = std::vector<std::vector<double> > (3,dumb);
        m_weights[0][1] = 1;
        m_weights[0][2] = 1;
        m_weights[2][1] = 1;
        m_weights[1][0] = 1;
        m_weights[2][0] = 1;
        m_weights[1][2] = 1;

        m_values = std::vector<double>(3,1.0);
        m_min_value = 1;
    }

    /// Constructor
    /**
     * Constructs a City-Selction TSP from the weights/values definition, a maximum
     * path length and the chosen encoding
     *
     * @param[in] weights         an std::vector of std::vector representing the edges weights
     * @param[in] values          an std::vector representing the vertices values
     * @param[in] max_path_length the maximum path length allowed (for the travelling salesman)
     * @param[in] encoding        a pagmo::problem::tsp::encoding representing the chosen encoding
     */
    tsp_cs::tsp_cs(const std::vector<std::vector<double> >& weights, const std::vector<double>& values, const double max_path_length, const base_tsp::encoding_type & encoding):
        base_tsp(weights.size(), 
            compute_dimensions(weights.size(), encoding)[0],
            compute_dimensions(weights.size(), encoding)[1],
            encoding
        ),  m_weights(weights), m_values(values), m_max_path_length(max_path_length)
    {
        check_weights(m_weights);
        if (weights.size() != values.size()) 
        {
            pagmo_throw(value_error,"Size of weight matrix and values vector must be equal");
        }
        m_min_value = *std::min(m_values.begin(), m_values.end());
    }

    /// Clone method.
    base_ptr tsp_cs::clone() const
    {
        return base_ptr(new tsp_cs(*this));
    }

    /// Checks if we can instantiate a TSP or ATSP problem
    /**
     * Checks if a matrix (std::vector<std::vector<double>>) 
     * is square or bidirectional (e.g. no one way links between vertices).
     * If none of the two conditions are true, we can not have a tsp problem.
     * @param[in] matrix - the adjacency matrix (two dimensional std::vector)
     * @throws value_error if matrix is not square and/or graph is not bidirectional
     */
    void tsp_cs::check_weights(const std::vector<std::vector<double> > &matrix) const 
    {   
        decision_vector::size_type n_cols = matrix.size();
        
        for (decision_vector::size_type i = 0; i < n_cols; ++i) {
            decision_vector::size_type n_rows = matrix.at(i).size();
            // check if the matrix is square
            if (n_rows != n_cols)
                pagmo_throw(value_error, "adjacency matrix is not square");
            
            for (size_t j = 0; j < n_rows; ++j) {
                if (i == j && matrix.at(i).at(j) != 0)
                    pagmo_throw(value_error, "main diagonal elements must all be zeros.");
                if (i != j && !matrix.at(i).at(j)) // fully connected
                    pagmo_throw(value_error, "adjacency matrix contains zero values.");
                if (i != j && (!matrix.at(i).at(j)) == matrix.at(i).at(j)) // fully connected
                    pagmo_throw(value_error, "adjacency matrix contains NaN values.");                    
            }
        }
    }

    boost::array<int, 2> tsp_cs::compute_dimensions(decision_vector::size_type n_cities, base_tsp::encoding_type encoding)
    {
        boost::array<int,2> retval;
        switch( encoding ) {
            case FULL:
                retval[0] = n_cities*(n_cities-1)+2;
                retval[1] = (n_cities-1)*(n_cities-2);
                break;
            case RANDOMKEYS:
                retval[0] = 0;
                retval[1] = 0;
                break;
            case CITIES:
                retval[0] = 1;
                retval[1] = 0;
                break;
        }
        return retval;
    }

    void tsp_cs::objfun_impl(fitness_vector &f, const decision_vector& x) const 
    {
        f[0]=0;
        decision_vector tour;
        decision_vector::size_type n_cities = get_n_cities();
        double cum_p, saved_length;
        decision_vector::size_type dumb1, dumb2;

        switch( get_encoding() ) {
            case FULL:
            {
                tour = full2cities(x);
                break;
            }
            case RANDOMKEYS:
            {
                tour = randomkeys2cities(x);
                break;
           }
            case CITIES:
           {
                tour = x;
                break;
           }
        }
        find_city_subsequence(tour, cum_p, saved_length, dumb1, dumb2);
        f[0] = -(cum_p + (1 - m_min_value) * n_cities + saved_length / m_max_path_length);
        return;
    }

    size_t tsp_cs::compute_idx(const size_t i, const size_t j, const size_t n) const
    {
        pagmo_assert( i!=j && i<n && j<n );
        return i*(n-1) + j - (j>i? 1:0);
    }


    /// Computes the best subpath of an hamilonian path satisfying the max_path_length constraint
    /**
     * Computes the best subpath of an hamilonian path satisfying the max_path_length constraint.
     * If the input tour does not represent an Hamiltonian path, (i.e. its an unfeasible chromosome)
     * the algorithm behaviour is undefined
     *
     * @param[in]  tour the hamiltonian path encoded with a CITIES encoding (i.e. the list of cities ids)
     * @param[out] retval_p the total cumulative value of the subpath
     * @param[out] retval_l the total cumulative length of the subpath
     * @param[out] retval_it_l the id of the city where the subpath starts
     * @param[out] retval_it_r the id of the city where the subpath ends
     * @throws value_error if the input tour length is not equal to the city number

     */
    void tsp_cs::find_city_subsequence(const decision_vector& tour, double& retval_p, double& retval_l, decision_vector::size_type& retval_it_l, decision_vector::size_type& retval_it_r) const
    {
        if (tour.size() != get_n_cities())
        {
            pagmo_throw(value_error, "tour dimension must be equal to the city number");
        }

        // We declare the necessary variable
        decision_vector::size_type n_cities = get_n_cities();
        decision_vector::size_type it_l = 0, it_r = 0;
        bool cond_r = true, cond_l = true;
        double cum_p = m_values[tour[0]];
        double saved_length = m_max_path_length;

        // We initialize the starting values
        retval_p = cum_p;
        retval_l = saved_length;
        retval_it_l = it_l;
        retval_it_r = it_r;

        // Main body of the double loop

        while(cond_l)
        {
            while(cond_r) 
            {
                // We increment the right "pointer" updating the value and length of the path
                saved_length -= m_weights[tour[it_r % n_cities]][tour[(it_r + 1) % n_cities]];
                cum_p += m_values[(it_r + 1) % n_cities];
                it_r += 1;

                // We update the various retvals only if the new subpath is valid
                if (saved_length < 0 || (it_l % n_cities == it_r % n_cities))
                {
                    cond_r = false;
                }
                else if (cum_p > retval_p)
                {
                    retval_p = cum_p;
                    retval_l = saved_length;
                    retval_it_l = it_l % n_cities;
                    retval_it_r = it_r % n_cities;
                }
                else if (cum_p == retval_p)
                {
                    if (saved_length > retval_l)
                    {
                        retval_p = cum_p;
                        retval_l = saved_length;
                        retval_it_l = it_l % n_cities;
                        retval_it_r = it_r % n_cities;
                    }
                }
            }
            // We get out if all cities are included in the current path
            if (it_l % n_cities == it_r % n_cities)
            {
                cond_l = false;
            }
            else
            {
                // We increment the left "pointer" updating the value and length of the path
                saved_length += m_weights[tour[it_l % n_cities]][tour[(it_l + 1) % n_cities]];
                cum_p -= m_values[it_l];
                it_l += 1;
                // We update the various retvals only if the new subpath is valid
                if (saved_length > 0)
                {
                    cond_r = true;
                    if (cum_p > retval_p)
                    {
                        retval_p = cum_p;
                        retval_l = saved_length;
                        retval_it_l = it_l % n_cities;
                        retval_it_r = it_r % n_cities;
                    }
                    else if (cum_p == retval_p)
                    {
                        if (saved_length > retval_l)
                        {
                            retval_p = cum_p;
                            retval_l = saved_length;
                            retval_it_l = it_l % n_cities;
                            retval_it_r = it_r % n_cities;
                        }
                    }
                }
                if (it_l == n_cities)
                {
                    cond_l = false;
                }
            }
        }
    }

    void tsp_cs::compute_constraints_impl(constraint_vector &c, const decision_vector& x) const 
    {
        decision_vector::size_type n_cities = get_n_cities();

        switch( get_encoding() ) 
        {
            case FULL:
            {
                // 1 - We set the equality constraints
                for (size_t i = 0; i < n_cities; i++) {
                    c[i] = 0;
                    c[i+n_cities] = 0;
                    for (size_t j = 0; j < n_cities; j++) {
                        if(i==j) continue; // ignoring main diagonal
                        decision_vector::size_type rows = compute_idx(i, j, n_cities);
                        decision_vector::size_type cols = compute_idx(j, i, n_cities);
                        c[i] += x[rows];
                        c[i+n_cities] += x[cols];
                    }
                    c[i] = c[i]-1;
                    c[i+n_cities] = c[i+n_cities]-1;
                }

                //2 - We set the inequality constraints
                //2.1 - First we compute the uj (see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Integer_linear_programming_formulation)
                //      we start always out tour from the first city, without loosing generality
                size_t next_city = 0,current_city = 0;
                std::vector<int> u(n_cities);
                for (size_t i = 0; i < n_cities; i++) {
                    u[current_city] = i+1;
                    for (size_t j = 0; j < n_cities; j++) 
                    {
                        if (current_city==j) continue;
                        if (x[compute_idx(current_city, j, n_cities)] == 1) 
                        {
                            next_city = j;
                            break;
                        }
                    }
                    current_city = next_city;
                }
                int count=0;
                for (size_t i = 1; i < n_cities; i++) {
                    for (size_t j = 1; j < n_cities; j++) 
                    {
                        if (i==j) continue;
                        c[2*n_cities+count] = u[i]-u[j] + (n_cities+1) * x[compute_idx(i, j, n_cities)] - n_cities;
                        count++;
                    }
                }
                break;
            }
            case RANDOMKEYS:
                break;
            case CITIES:
            {
                std::vector<population::size_type> range(n_cities);
                for (std::vector<population::size_type>::size_type i=0; i<range.size(); ++i) 
                {
                    range[i]=i;
                }
                c[0] = !std::is_permutation(x.begin(),x.end(),range.begin());
                break;
            }
        }
        return;
    }

    /// Definition of distance function
    double tsp_cs::distance(decision_vector::size_type i, decision_vector::size_type j) const
    {
        return m_weights[i][j];
    }

    /// Getter for m_weights
    /**
     * @return const reference to m_weights
     */
    const std::vector<std::vector<double> >&  tsp_cs::get_weights() const
    { 
        return m_weights; 
    }

    /// Getter for m_values
    /**
     * @return const reference to m_values
     */
    const std::vector<double>&  tsp_cs::get_values() const
    { 
        return m_values; 
    }

    /// Getter for m_max_path_value
    /**
     * @return m_max_path_value
     */
    double  tsp_cs::get_max_path_length() const
    { 
        return m_max_path_length; 
    }

    /// Returns the problem name
    std::string tsp_cs::get_name() const
    {
        return "City-selection Travelling Salesman Problem (TSP-CS)";
    }

    /// Extra human readable info for the problem.
    /**
     * @return a std::string containing a list of vertices and edges
     */
    std::string tsp_cs::human_readable_extra() const 
    {
        std::ostringstream oss;
        oss << "\n\tNumber of cities: " << get_n_cities() << '\n';
        oss << "\tEncoding: ";
        switch( get_encoding()  ) {
            case FULL:
                oss << "FULL" << '\n';
                break;
            case RANDOMKEYS:
                oss << "RANDOMKEYS" << '\n';
                break;
            case CITIES:
                oss << "CITIES" << '\n';
                break;
        }
        oss << "\tCities Values: " << m_values << std::endl;
        oss << "\tMax path length: " << m_max_path_length << '\n';
        oss << "\tWeight Matrix: \n";
        for (decision_vector::size_type i=0; i<get_n_cities() ; ++i)
        {
            oss << "\t\t" << m_weights.at(i) << '\n';
            if (i>5)
            {
                oss << "\t\t..." << '\n';
                break;
            }
        }
        return oss.str();
    }

    
}} //namespaces

BOOST_CLASS_EXPORT_IMPLEMENT(pagmo::problem::tsp_cs)