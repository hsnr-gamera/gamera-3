/*
 * Copyright (C) 2012 Tobias Bolten
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef gamodule201206
#define gamodule201206

#include "knncoremodule.hpp"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>
#include <time.h>
#include <map>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <eo>
#include <es.h>

#include <ga/eoBitOp.h>
#include <eoStochasticUniversalSelect.h>
#include <eoShiftMutation.h>
#include <eoSwapMutation.h>
#include <eoTwoOptMutation.h>

namespace Gamera { namespace GA {

    typedef eoBit<double> SelectionIndi;
    typedef eoReal<double> WeightingIndi;

    enum OperationMode {GA_SELECTION, GA_WEIGHTING};

    template <typename EOT>
    class SelectOneDefaultWorth : public eoSelectOne<EOT> {};

    /**************************************************************************/
    template<class EOT>
    class GATwoOptMutation : public eoMonOp<EOT> {
    /**************************************************************************/
        public:
            typedef typename EOT::AtomType GeneType;

            GATwoOptMutation(){}

            virtual std::string className() const { return "GATwoOptMutation"; }

            bool operator()(EOT& _eo) {
                // generate two different indices
                unsigned int i = eo::rng.random(_eo.size());
                unsigned int j;

                do {
                    j = eo::rng.random(_eo.size());
                } while(i == j);

                unsigned int from = std::min(i,j);
                unsigned int to = std::max(i,j);
                unsigned int idx = (to - from) / 2;

                // inverse between from and to
                for(unsigned k = 0; k <= idx; ++k) {
                    GeneType tmp = _eo[from+k];
                    _eo[from+k] = _eo[to-k];
                    _eo[to-k] = tmp;
                }
                return true;
            }
    };

    /**************************************************************************/
    template<class EOT>
    class GASwapMutation : public eoMonOp<EOT> {
    /**************************************************************************/
        public:
            typedef typename EOT::AtomType GeneType;

            GASwapMutation(const unsigned _howManySwaps=1): howManySwaps(_howManySwaps) {
                if(howManySwaps < 1) {
                    throw std::runtime_error("Invalid number of swaps in GASwapMutation");
                }
            }

            virtual std::string className() const { return "GASwapMutation"; }

            bool operator()(EOT& chrom) {
                unsigned i, j;

                for(unsigned int swap = 0; swap < howManySwaps; swap++) {
                    // generate two different indices
                    i=eo::rng.random(chrom.size());

                    do {
                        j = eo::rng.random(chrom.size());
                    } while (i == j);

                    // swap
                    GeneType tmp = chrom[i];
                    chrom[i] = chrom[j];
                    chrom[j] = tmp;
              }
              return true;
            }

        private:
                unsigned int howManySwaps;
    };

    /**************************************************************************/
    template <typename EOT>
    class GAManualStop : public eoContinue<EOT> {
    /**************************************************************************/
        protected:
            bool continueFlag;

        public:
            GAManualStop() {
                this->continueFlag = true;
            }

            virtual bool operator() ( const eoPop<EOT>& _vEO ) {
                return this->continueFlag;
            }

            bool getFlag() {
                return this->continueFlag;
            }

            void setFlag(bool flag) {
                this->continueFlag = flag;
            }
    };

    /**************************************************************************/
    template <typename EOT>
    class GABestIndiStat : public eoStat<EOT, std::string> {
    /**************************************************************************/
        public:
            using eoStat<EOT, std::string>::value;

            GABestIndiStat(std::string name = "bestIndi")
            : eoStat<EOT, std::string>(std::string(""), name)
            {}

            void operator()(const eoPop<EOT> &pop) {
                const EOT bestIndi = pop.best_element();
                typename EOT::const_iterator it;

                std::ostringstream indiStream;

                indiStream << "[";
                for (it = bestIndi.begin(); it != bestIndi.end(); ++it) {
                    indiStream << *it << " , ";
                }
                indiStream << "]";

                value() = indiStream.str();
            }

            virtual std::string className(void) const { return "GABestIndiStat"; }
    };

    /**************************************************************************/
    template <typename EOT>
    class GAClassifierUpdater : public eoContinue<EOT> {
    /**************************************************************************/
        protected:
            KnnObject *knn;
            double bestFitness;
            std::vector<EOT> bestSolution;
            std::map<unsigned int, unsigned int> *indexRelation;

        public:
            GAClassifierUpdater(KnnObject *knn, std::map<unsigned int, unsigned int> *indexRelation) {
                this->knn = knn;
                this->bestFitness = 0.0;
                this->bestSolution.resize(this->knn->num_features);
                this->indexRelation = indexRelation;
            }

            double getBestFitness() {
                return this->bestFitness;
            }

            virtual bool operator()(const eoPop<EOT>& pop);

            virtual std::string className(void) const { return "GAClassifierUpdater"; }
    };

    template <>
    bool GAClassifierUpdater<WeightingIndi>::operator() (const eoPop<WeightingIndi> &pop) {
        const WeightingIndi bestIndi = pop.best_element();
        WeightingIndi::const_iterator it;

        if (bestIndi.fitness() > this->bestFitness) {
            this->bestFitness = bestIndi.fitness();

            std::fill(this->knn->weight_vector, this->knn->weight_vector + this->knn->num_features, 0.0);
            std::fill(this->bestSolution.begin(), this->bestSolution.end(), 0.0);

            for (size_t i = 0; i < bestIndi.size(); ++i) {
                this->knn->weight_vector[(*this->indexRelation)[i]] = bestIndi[i];
                this->bestSolution[(*this->indexRelation)[i]] = bestIndi[i];
            }
        }

        return true;
    }

    template <>
    bool GAClassifierUpdater<SelectionIndi>::operator() (const eoPop<SelectionIndi> &pop) {
        const SelectionIndi bestIndi = pop.best_element();
        SelectionIndi::const_iterator it;

        if (bestIndi.fitness() > this->bestFitness) {
            this->bestFitness = bestIndi.fitness();

            std::fill(this->knn->selection_vector, this->knn->selection_vector + this->knn->num_features, 0);
            std::fill(this->bestSolution.begin(), this->bestSolution.end(), false);

            for (size_t i = 0; i < bestIndi.size(); ++i) {
                this->knn->selection_vector[(*this->indexRelation)[i]] = bestIndi[i];
                this->bestSolution[(*this->indexRelation)[i]] = bestIndi[i];
            }
        }

        return true;
    }

    // *************************************************************************
    template <typename EOT>
    class GAFitnessEval : public eoEvalFunc<EOT> {
    // *************************************************************************
        protected:
            KnnObject *knn;
            std::map<unsigned int, unsigned int> *indexRelation;

            typedef typename EOT::ContainerType ContainerType;
            typedef typename EOT::AtomType AtomType;

        public:
            GAFitnessEval(KnnObject *knn, std::map<unsigned int, unsigned int> *indexRelation) {
                this->knn = knn;
                this->indexRelation = indexRelation;
            }

            virtual std::string className(void) const { return "GAFitnessEval"; }

            virtual void operator()( EOT &individual );
    };

    // specialization for weighting individual
    template <>
    void GAFitnessEval<WeightingIndi>::operator()( WeightingIndi &individual ) {
        AtomType convertedVector[this->knn->num_features];
        std::fill(convertedVector, convertedVector + this->knn->num_features, 0.0);

        for (size_t i = 0; i < individual.size(); ++i) {
            convertedVector[(*this->indexRelation)[i]] = individual[i];
        }

        std::pair<int, int> looEvalRes;
        looEvalRes = leave_one_out(this->knn, std::numeric_limits<int>::max(),
                                   NULL, convertedVector, NULL);

        individual.fitness( looEvalRes.first / (double) looEvalRes.second );
    }

    // specialization for selection individual
    template <>
    void GAFitnessEval<SelectionIndi>::operator()( SelectionIndi &individual ) {
        int convertedVector[this->knn->num_features];
        std::fill(convertedVector, convertedVector + this->knn->num_features, 0);

        for (size_t i = 0; i < individual.size(); ++i) {
            // §4.7/4 from the C++ Standard (Integral Conversion):
            // If the source type is bool, the value false is converted to zero
            // and the value true is converted to one.
            convertedVector[(*this->indexRelation)[i]] = (int) individual[i];
        }

        std::pair<int, int> looEvalRes;
        looEvalRes = leave_one_out(this->knn, std::numeric_limits<int>::max(),
                                   convertedVector, NULL, NULL);

        individual.fitness( looEvalRes.first / (double) looEvalRes.second );
    }

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO>
    class GAMultiSettingBase {
    /**************************************************************************/
        protected:
            std::vector<EO<EOT>*> *settings;

        public:
            GAMultiSettingBase<EOT, EO>();
            ~GAMultiSettingBase<EOT, EO>();

            std::vector<EO<EOT>*> * getSettings();
    };

    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO>
    class GASingleSettingBase {
    /**************************************************************************/
        protected:
            EO<EOT> *setting;

        public:
            GASingleSettingBase<EOT, EO>();
            ~GASingleSettingBase<EOT, EO>();

            EO<EOT> * getSetting();
    };

    /**************************************************************************/
    class GABaseSetting {
    /**************************************************************************/
        protected:
            int opMode;
            unsigned int pSize;
            double cRate;
            double mRate;

        public:
            GABaseSetting(int opMode = GA_SELECTION,
                          unsigned int pSize = 75,
                          double cRate = 0.95, double mRate = 0.05);
            // getter
            int getOpMode();
            unsigned int getPopSize();
            double getCrossRate();
            double getMutRate();

            // setter
            void setOpMode(int opMode);
            void setPopSize(unsigned int pSize);
            void setCrossRate(double cRate);
            void setMutRate(double mRate);
    };

    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO = SelectOneDefaultWorth>
    class GASelection : public GASingleSettingBase<EOT, EO> {
    /**************************************************************************/
        public:
            void setRoulettWheel();
            void setRoulettWheelScaled(double preasure = 2.0);
            void setStochUniSampling();
            void setRankSelection(double preasure = 2.0, double exponent = 1.0);
            void setTournamentSelection(unsigned int tSize = 3);
            void setRandomSelection();
    };

    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO = eoQuadOp>
    class GACrossover : public GAMultiSettingBase<EOT, EO> {
    /**************************************************************************/
        protected:
            eoRealVectorBounds *bound;

        public:
            GACrossover();
            ~GACrossover();

            // generic functions
            void setNPointCrossover(unsigned int n = 1);
            void setUniformCrossover(double preference = 0.5);

            // weighting functions
            void setSBXcrossover(unsigned int numFeatures, double min,
                                 double max, double eta = 1.0);
            void setSegmentCrossover(unsigned int numFeatures, double min,
                                     double max, double alpha = 0.0);
            void setHypercubeCrossover(unsigned int numFeatures, double min,
                                       double max, double alpha = 0.0);
    };
    
    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO = eoMonOp>
    class GAMutation : public GAMultiSettingBase<EOT, EO> {
    /**************************************************************************/
        protected:
            eoRealVectorBounds *bound;

        public:
            GAMutation();
            ~GAMutation();

            void setShiftMutation();
            void setSwapMutation();
            void setInversionMutation();

            // selection functions
            void setBinaryMutation(double rate = 0.05, bool normalize = false);

            // weighting functions
            void setGaussMutation(unsigned int numFeatures, double min,
                                  double max, double sigma, double p_change = 1.0);
    };

    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO = eoReplacement>
    class GAReplacement : public GASingleSettingBase<EOT, EO> {
    /**************************************************************************/
        public:
            void setGenerationalReplacement();
            void setSSGAworse();
            void setSSGAdetTournament(unsigned int tSize = 3);
    };
    
    /**************************************************************************/
    template <typename EOT, template <typename IndiType> class EO = eoContinue>
    class GAStopCriteria : public GAMultiSettingBase<EOT, EO> {
    /**************************************************************************/
        public:
            void setBestFitnessStop(double optimum = 1.0);
            void setMaxGenerations(unsigned int n = 100);
            void setMaxFitnessEvals(unsigned int n = 5000);
            void setSteadyStateStop(unsigned int minGens = 40, unsigned int noChangeGens = 10);
    };

    /**************************************************************************/
    class GAParallelization {
    /**************************************************************************/
        protected:
            bool parallelMode;
            unsigned int threadNum;

        public:
            GAParallelization(bool mode = true, unsigned int threads = 2);

            bool isParallel();
            void changeMode(bool mode = true);

            unsigned int getThreadNum();
            void setThreadNum(unsigned int n = 2);
    };

    /**************************************************************************/
    template <typename EOT>
    class GAOptimization {
    /**************************************************************************/
        protected:
            bool running;

            // basic GA settings
            unsigned int popSize;
            double crossoverRate;
            double mutationRate;

            KnnObject *knn;
            GABaseSetting *baseSetting;
            GASelection<EOT> *selection;
            GACrossover<EOT> *crossover;
            GAMutation<EOT> *mutation;
            GAReplacement<EOT> *replacement;
            GAStopCriteria<EOT> *stop;
            GAParallelization *parallelization;

            GAManualStop<EOT> manualStop;

            eoIncrementorParam<unsigned int> *generationCounter;
            eoBestFitnessStat<EOT> *bestStat;
            GAClassifierUpdater<EOT> *kNNUpdater;

            std::ostringstream *monitorStream;
            std::ostringstream *bestIndiStream;

        public:
            GAOptimization<EOT>(KnnObject *knn,
                           GABaseSetting *baseSetting,
                           GASelection<EOT> *selection,
                           GACrossover<EOT> *crossover,
                           GAMutation<EOT> *mutation,
                           GAReplacement<EOT> *replacement,
                           GAStopCriteria<EOT> *stop,
                           GAParallelization *parallel);
            ~GAOptimization<EOT>();

            void StartCalculation();
            void StopCalculation();

            // getter
            bool getRunStatus();

            KnnObject *getKnnObject();

            GABaseSetting * getBaseSetting();
            GASelection<EOT> * getSelection();
            GACrossover<EOT> * getCrossover();
            GAMutation<EOT> * getMutation();
            GAReplacement<EOT> * getReplacement();
            GAStopCriteria<EOT> * getStopCriteria();
            GAParallelization * getParallelization();

            unsigned int getGenerationCount();
            double getBestFitnessValue();
            std::string getMonitorString();
            std::string getBestIndiString();

            // setter
            void setKnnObject(KnnObject *knn);

            void setBaseSetting(GABaseSetting * baseSetting);
            void setSelection(GASelection<EOT> *selection);
            void setCrossover(GACrossover<EOT> *crossover);
            void setMutation(GAMutation<EOT> *mutation);
            void setReplacement(GAReplacement<EOT> *replacement);
            void setStopCriteria(GAStopCriteria<EOT> *stop);
            void setParallelization(GAParallelization *parallel);
    };

#include "knnga.cpp"

}} // end of namespaces

#endif
