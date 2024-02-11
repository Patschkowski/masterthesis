//Copyright (C) 2013 Cem Bassoy.
//
//This file is part of the OpenCL Utility Toolkit.
//
//OpenCL Utility Toolkit is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//OpenCL Utility Toolkit is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with OpenCL Utility Toolkit.  If not, see <http://www.gnu.org/licenses/>.

#ifndef UTL_PROFILE_PASS_H
#define UTL_PROFILE_PASS_H


#include <ostream>
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <chrono>

#include <utl_stream.h>
#include <utl_dim.h>
#include <utl_timer.h>




namespace utl{

/*! \class ProfilePass utl_profile_pass.h "inc/utl_profile_pass.h"
  * \brief Utility class to profile functions.
  *
  * The ProfilePass class can be inherited to profile a function.
  * Objects from inherited classes of ProfilePass can be stored in a
  * ProfilePassManager object which runs all ProfilePass functions.
  * Inhertied objects need to implement the prof and ops functions.
  */

template <class T>
class ProfilePass
{

public:
	ProfilePass(const std::string& str, const Dim& start, const Dim& step, const Dim& end, size_t __iter = 10) :
        _name(str), _start(start), _step(step), _end(end), _iter(__iter), _print_n(true), _print_t(true), _print_o(true), _print_p(true) , _countUp(true)
	{}
	
	using ValueType = T;
	
	ProfilePass() = delete;
	ProfilePass(const ProfilePass&) = default;
	ProfilePass(ProfilePass&&) = default;
  virtual ~ProfilePass(){}

	template<class F>
	double call(F&& __func)
	{
        utl::Timer::tic();
		for (size_t j = 0; j < _iter; j++){
			__func();
            //std::cout << "Iter=" << _iter << ", cIter = " << j << std::endl;
		}
        utl::Timer::toc();
        return utl::Timer::elapsed(_iter);
	}


    virtual double prof(const Dim&) = 0;
    virtual double ops(const Dim&) = 0;


	void run()
    {
        //size_t j = 0;

    std::function<bool(const Dim&, const Dim&)> compare;
    std::function<void(Dim&, const Dim&)> advance;

    if (_countUp) {
      compare = [](const Dim& lhs, const Dim& rhs) { return (lhs < 1) == 0 && lhs <= rhs; };
      advance = [](Dim& lhs, const Dim& rhs) { lhs += rhs; };
    }
    else {
      compare = [](const Dim& lhs, const Dim& rhs) { return (lhs < 1) == 0 && lhs >= rhs; };
      advance = [](Dim& lhs, const Dim& rhs) { lhs -= rhs; };
    }

        //for(Dim i = _start; i <= _end; i += _step, ++j)
        for(Dim i = _start; compare(i, _end); advance(i,_step))
        {
    //TRUE_COMMENT("start : " << this->_start.toString() << ", _end : " << this->_end.toString() << ", _step = " << this->_step.toString() << ", i " << i.toString() << ", comp = "  << compare(i,_end));

            double time = this->prof(i); // seconds
            double op = this->ops(i);
			double perf = double(op)  / time;

            this->_elems.push_back(i.prod());
            this->_times.push_back(time) ;
            this->_ops.push_back(op) ; // 2 * n^2 + n
            this->_perf.push_back(perf);
		}

	}


    const std::vector<double> & dims()    const { return _elems; }
	const std::vector<double> & times()   const { return _times; }
	const std::vector<double> & ops()     const { return _ops; }
	const std::vector<double> & perf()    const { return _perf; }
	const std::string &name()             const { return _name; }

    void setCountUp()   { this->_countUp = true; }
    void setCountDown() { this->_countUp = false; }

    void setPrint(bool print_n, bool print_t, bool print_o, bool print_p)
    {
        _print_n = print_n;
        _print_t = print_t;
        _print_o = print_o;
        _print_p = print_p;
    }

    void setName(const std::string& n)  { _name = n; }
    void setIter(size_t iter) { this->_iter = iter; }

	template<class E>
	std::string toString(const std::vector<E> &v) const
	{
		if(v.empty()) return "[];";
                
                std::ostringstream oss;
                oss.precision( std::numeric_limits< E >::digits10 );
                oss << std::scientific << '[';
                
                for ( size_t i = 0; i < v.size()-1u; ++i )
                {
                  oss << v.at(i) << ',';
                }
                
                oss << v.back() << "];";
		return oss.str();
	}


	friend std::ostream& operator <<(std::ostream &out, const ProfilePass<T> *_p)
	{
		out.precision(std::numeric_limits< double >::digits10);
		out << std::scientific;
		if(_p->_print_n) out << _p->name() << "_n = " << _p->toString<double>(_p->dims())  << std::endl;
        if(_p->_print_o) out << _p->name() << "_o = " << _p->toString<double>(_p->ops())   << std::endl;
		if(_p->_print_t) out << _p->name() << "_t = " << _p->toString<double>(_p->times()) << std::endl;
        if(_p->_print_p) out << _p->name() << "_p = " << _p->toString<double>(_p->perf())  << std::endl;
		return out;
	}

protected:
	std::string _name;
    utl::Dim _start, _step, _end;
	size_t _iter;
    bool _print_n, _print_t, _print_o, _print_p;

    std::vector<double> _elems;
	std::vector<double> _ops;   /*!< Number of operations at each dimension [ ops ]*/
	std::vector<double> _times; /*!< Time needed for the operation [ s ] */
	std::vector<double> _perf;  /*!< Number of operations per Time [ Ops / s ]*/

    bool _countUp;

};

}

#endif
