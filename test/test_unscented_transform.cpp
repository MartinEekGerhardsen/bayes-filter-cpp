/*
 * test_unscented_transform.cpp
 *
 *  Created on: 12 Jun 2018
 *      Author: Fabian Meyer
 */

#include <catch.hpp>
#include "bayes_filter/unscented_transform.h"
#include "eigen_assert.h"

using namespace bf;

static Eigen::VectorXd identityTransform(const Eigen::VectorXd &state)
{
    return state;
}


static Eigen::VectorXd linearTransform(const Eigen::VectorXd &state, const Eigen::VectorXd &facs)
{
    assert(state.rows() == facs.rows());

    Eigen::VectorXd result(state.size());
    for(unsigned int i = 0; i < state.rows(); ++i)
        result(i) = state(i) * facs(i);

    return result;
}

static Eigen::MatrixXd linearTransformCov(const Eigen::MatrixXd &cov, const Eigen::VectorXd &facs)
{
    assert(cov.rows() == facs.rows());

    Eigen::MatrixXd result = Eigen::MatrixXd::Zero(cov.rows(), cov.cols());
    for(unsigned int i = 0; i < cov.rows(); ++i)
        result(i, i) = cov(i,i) * facs(i) * facs(i);

    return result;
}

static SigmaPoints linearTransformSig(SigmaPoints &sigma, const Eigen::VectorXd &facs)
{
    assert(facs.rows() == sigma.points.rows());

    SigmaPoints result;
    result = sigma;
    for(unsigned int i = 0; i < sigma.points.cols(); ++i)
    {
        result.points.col(i) = linearTransform(result.points.col(i), facs);
    }

    return result;
}

TEST_CASE("Unscented Transform")
{
    /* =====================================================================
     *      Sigma Points
     * ===================================================================== */
    SECTION("calculate sigma points")
    {
        const double eps = 1e-6;
        UnscentedTransform trans;

        SECTION("with simple params")
        {
            trans.setAlpha(1.0);
            trans.setBeta(1.0);
            trans.setKappa(1.0);

            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;

            Eigen::MatrixXd wexp(2, 7);
            wexp << 0.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125,
                    1.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125;

            Eigen::MatrixXd sexp(3, 7);
            sexp << 1, 3, 1, 1, -1,  1,  1,
                    1, 1, 3, 1,  1, -1,  1,
                    1, 1, 1, 3,  1,  1, -1;

            auto result = trans.calcSigmaPoints(state, cov);

            REQUIRE(trans.calcLambda(state.size()) == Approx(1.0).margin(eps));
            REQUIRE_MAT(wexp, result.weights, eps);
            REQUIRE_MAT(sexp, result.points, eps);
        }

        SECTION("with different params")
        {
            trans.setAlpha(1.0);
            trans.setBeta(2.0);
            trans.setKappa(2.0);

            Eigen::VectorXd state(2);
            state << 1,1;
            Eigen::MatrixXd cov(2,2);
            cov << 1, 0,
                   0, 1;

            Eigen::MatrixXd wexp(2, 5);
            wexp << 0.5, 0.125, 0.125, 0.125, 0.125,
                    2.5, 0.125, 0.125, 0.125, 0.125;

            Eigen::MatrixXd sexp(2, 5);
            sexp << 1, 3, 1, -1,  1,
                    1, 1, 3,  1, -1;

            auto result = trans.calcSigmaPoints(state, cov);

            REQUIRE(trans.calcLambda(state.size()) == Approx(2.0).margin(eps));
            REQUIRE_MAT(wexp, result.weights, eps);
            REQUIRE_MAT(sexp, result.points, eps);
        }

        SECTION("with zero uncertainty")
        {
            trans.setAlpha(1.0);
            trans.setBeta(1.0);
            trans.setKappa(1.0);

            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 0, 0,
                   0, 0, 0;

            Eigen::MatrixXd wexp(2, 7);
            wexp << 0.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125,
                    1.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125;

            Eigen::MatrixXd sexp(3, 7);
            sexp << 1, 3, 1, 1, -1,  1,  1,
                    1, 1, 1, 1,  1,  1,  1,
                    1, 1, 1, 1,  1,  1,  1;

            auto result = trans.calcSigmaPoints(state, cov);

            REQUIRE(trans.calcLambda(state.size()) == Approx(1.0).margin(eps));
            REQUIRE_MAT(wexp, result.weights, eps);
            REQUIRE_MAT(sexp, result.points, eps);
        }
    }

    /* =====================================================================
     *      Recover Distribution
     * ===================================================================== */

    SECTION("recover distribution")
    {
        const double eps = 1e-6;
        UnscentedTransform trans;

        SECTION("with identity transform")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;

            auto sigma = trans.calcSigmaPoints(state, cov);
            auto result = trans.recoverDistrib(sigma);

            REQUIRE_MAT(state, result.first, eps);
            REQUIRE_MAT(cov, result.second, eps);
        }

        SECTION("with identity transform and near zero uncertainty")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1e-16, 0, 0,
                   0, 1, 0,
                   0, 0, 1e-16;

            auto sigma = trans.calcSigmaPoints(state, cov);
            auto result = trans.recoverDistrib(sigma);

            REQUIRE_MAT(state, result.first, eps);
            REQUIRE_MAT(cov, result.second, eps);
        }

        SECTION("with linear transform")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;
            Eigen::VectorXd facs(3);
            facs << 1, 2, 3;

            auto sigma = trans.calcSigmaPoints(state, cov);
            sigma = linearTransformSig(sigma, facs);

            auto result = trans.recoverDistrib(sigma);

            state = linearTransform(state, facs);
            cov = linearTransformCov(cov, facs);

            REQUIRE_MAT(state, result.first, eps);
            REQUIRE_MAT(cov, result.second, eps);
        }

        SECTION("with linear transform and near zero uncertainty")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1e-16, 0, 0,
                   0, 1, 0,
                   0, 0, 1e-16;
            Eigen::VectorXd facs(3);
            facs << 1, 2, 3;

            auto sigma = trans.calcSigmaPoints(state, cov);
            sigma = linearTransformSig(sigma, facs);

            auto result = trans.recoverDistrib(sigma);

            state = linearTransform(state, facs);
            cov = linearTransformCov(cov, facs);

            REQUIRE_MAT(state, result.first, eps);
            REQUIRE_MAT(cov, result.second, eps);
        }
    }

    /* =====================================================================
     *      Cross Covariance
     * ===================================================================== */

    SECTION("caclulate cross covariance")
    {
        const double eps = 1e-6;
        UnscentedTransform trans;

        SECTION("with identity transform")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;

            auto sigma = trans.calcSigmaPoints(state, cov);
            auto result = trans.calcCrossCov(state, sigma, state, sigma);

            REQUIRE_MAT(cov, result, eps);
        }

        SECTION("with linear transform")
        {
            Eigen::VectorXd state1(3);
            state1 << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;
            Eigen::VectorXd facs(3);
            facs << 1, 2, 3;
            Eigen::VectorXd state2 = linearTransform(state1, facs);

            Eigen::MatrixXd crossCov(3,3);
            crossCov << 1, 0, 0,
                        0, 2, 0,
                        0, 0, 3;

            auto sigma1 = trans.calcSigmaPoints(state1, cov);
            auto sigma2 = linearTransformSig(sigma1, facs);
            auto result = trans.calcCrossCov(state1, sigma1, state2, sigma2);

            REQUIRE_MAT(crossCov, result, eps);
        }

        SECTION("with linear transform and near zero uncertainty")
        {
            Eigen::VectorXd state1(3);
            state1 << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1e-16, 0, 0,
                   0, 1, 0,
                   0, 0, 1e-16;
            Eigen::VectorXd facs(3);
            facs << 1, 2, 3;
            Eigen::VectorXd state2 = linearTransform(state1, facs);

            Eigen::MatrixXd crossCov(3,3);
            crossCov << 0, 0, 0,
                      0, 2, 0,
                      0, 0, 0;

            auto sigma1 = trans.calcSigmaPoints(state1, cov);
            auto sigma2 = linearTransformSig(sigma1, facs);
            auto result = trans.calcCrossCov(state1, sigma1, state2, sigma2);

            REQUIRE_MAT(crossCov, result, eps);
        }
    }

    /* =====================================================================
     *      Transform Function
     * ===================================================================== */

    SECTION("apply transform function")
    {
        const double eps = 1e-6;
        UnscentedTransform trans;

        SECTION("with identity transform")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;

            auto result = trans.transform(
                state,
                cov,
                std::bind(identityTransform, std::placeholders::_1),
                true);

            REQUIRE_MAT(state, result.state, eps);
            REQUIRE_MAT(cov, result.cov, eps);
            REQUIRE_MAT(cov, result.crossCov, eps);
        }

        SECTION("with linear transform")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1, 0, 0,
                   0, 1, 0,
                   0, 0, 1;
            Eigen::VectorXd facs(3);
            facs << 1, 2, 3;
            Eigen::MatrixXd crossCov(3,3);
            crossCov << 1, 0, 0,
                        0, 2, 0,
                        0, 0, 3;

            auto result = trans.transform(
                state,
                cov,
                std::bind(linearTransform, std::placeholders::_1, std::cref(facs)),
                true);

            state = linearTransform(state, facs);
            cov = linearTransformCov(cov, facs);

            REQUIRE_MAT(state, result.state, eps);
            REQUIRE_MAT(cov, result.cov, eps);
            REQUIRE_MAT(crossCov, result.crossCov, eps);
        }

        SECTION("with linear transform and near zero uncertainty")
        {
            Eigen::VectorXd state(3);
            state << 1, 1, 1;
            Eigen::MatrixXd cov(3,3);
            cov << 1e-16, 0, 0,
                   0, 1, 0,
                   0, 0, 1e-16;
            Eigen::VectorXd facs(3);
            facs << 1, 2, 3;
            Eigen::MatrixXd crossCov(3,3);
            crossCov << 0, 0, 0,
                        0, 2, 0,
                        0, 0, 0;

            auto result = trans.transform(
                state,
                cov,
                std::bind(linearTransform, std::placeholders::_1, std::cref(facs)),
                true);

            state = linearTransform(state, facs);
            cov = linearTransformCov(cov, facs);

            REQUIRE_MAT(state, result.state, eps);
            REQUIRE_MAT(cov, result.cov, eps);
            REQUIRE_MAT(crossCov, result.crossCov, eps);
        }
    }
}
