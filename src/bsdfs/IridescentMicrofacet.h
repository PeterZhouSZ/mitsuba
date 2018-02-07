#pragma once

MTS_NAMESPACE_BEGIN

/* Evaluate a continuous spectrum at specific values. Since MTS internal
 * function is not accessible, it is copy/pasted here.
 */
Spectrum fromContinuousSpectrum(const ContinuousSpectrum &smooth,
                                const Spectrum &m_wavelengths)
{
      Spectrum s;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
            s[i] = smooth.eval(m_wavelengths[i]);
      return s;
}

/* Helper function: compute the square root of a spectrum variable */
Spectrum sqrt(const Spectrum &s)
{
      return s.sqrt();
}

/* Helper function: compute the power of a spectrum variable */
Spectrum sqr(const Spectrum &s)
{
      Spectrum value;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
            value[i] = s[i] * s[i];
      return value;
}
Float sqr(const Float &s)
{
      return s * s;
}

/* Helper function: compute the expoenential of a spectrum variable */
Spectrum exp(const Spectrum &s)
{
      Spectrum value;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
            value[i] = std::exp(s[i]);
      return value;
}

/* Helper function: compute the sine of a spectrum variable */
Spectrum sin(const Spectrum &s)
{
      Spectrum value;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
            value[i] = std::sin(s[i]);
      return value;
}

/* Helper function: compute the cosine of a spectrum variable */
Spectrum cos(const Spectrum &s)
{
      Spectrum value;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
            value[i] = std::cos(s[i]);
      return value;
}

/* Helper function: compute the atan of a spectrum variable */
Spectrum atan(const Spectrum &y, const Spectrum &x)
{
      Spectrum value;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
            value[i] = std::atan2(y[i], x[i]);
      return value;
}

/* Helper function: compute the atan of a spectrum variable */
Spectrum clamp(const Spectrum &x)
{
      Spectrum value;
      for (int i = 0; i < SPECTRUM_SAMPLES; i++)
      {
            value[i] = std::min<Float>(std::max<Float>(x[i], 0.0), 1.0);
      }
      return value;
}

/* Phase shift due to a conducting material.
 * See our appendix
 */
inline void fresnelPhaseExact(const Spectrum &cost,
                              const Spectrum &eta1,
                              const Spectrum &eta2, const Spectrum &kappa2,
                              Spectrum &phiP, Spectrum &phiS)
{
      const Spectrum sinThetaSqr = Spectrum(1.0) - sqr(cost);
      const Spectrum A = sqr(eta2) * (Spectrum(1.0) - sqr(kappa2)) - sqr(eta1) * sinThetaSqr;
      const Spectrum B = sqrt(sqr(A) + sqr(2 * sqr(eta2) * kappa2));
      const Spectrum U = sqrt((A + B) / 2.0);
      const Spectrum V = sqrt((B - A) / 2.0);

      phiS = atan(2 * eta1 * V * cost, sqr(U) + sqr(V) - sqr(eta1 * cost));
      phiP = atan(2 * eta1 * sqr(eta2) * cost * (2 * kappa2 * U - (Spectrum(1.0) - sqr(kappa2)) * V),
                  sqr(sqr(eta2) * (Spectrum(1.0) + sqr(kappa2)) * cost) - sqr(eta1) * (sqr(U) + sqr(V)));
}

/* Polarized Fresnel Term
*/
void fresnelConductorExact(Float cosThetaI,
                           Float eta, Float k,
                           Float &Rp2, Float &Rs2)
{
      /* Modified from "Optics" by K.D. Moeller, University Science Books, 1988 */

      Float cosThetaI2 = cosThetaI * cosThetaI,
            sinThetaI2 = 1 - cosThetaI2,
            sinThetaI4 = sinThetaI2 * sinThetaI2;

      Float temp1 = eta * eta - k * k - sinThetaI2,
            a2pb2 = math::safe_sqrt(temp1 * temp1 + 4 * k * k * eta * eta),
            a = math::safe_sqrt(0.5f * (a2pb2 + temp1));

      Float term1 = a2pb2 + cosThetaI2,
            term2 = 2 * a * cosThetaI;

      Rs2 = (term1 - term2) / (term1 + term2);

      Float term3 = a2pb2 * cosThetaI2 + sinThetaI4,
            term4 = term2 * sinThetaI2;

      Rp2 = Rs2 * (term3 - term4) / (term3 + term4);
}

#if SPECTRUM_SAMPLES == 3
/* Gaussian fits of the Fourier Transforms of the normalized XYZ profiles
 */
// const Float _val[3] = {5.4856e-13, 4.4201e-13, 5.2481e-13};
const Float _val[3] = {3.8789e-13, 3.1255e-13, 3.7110e-13};
const Float _pos[3] = {1.6810e+06, 1.7953e+06, 2.2084e+06};
// const Float _var[3] = {4.3278e+09, 9.3046e+09, 6.6121e+09};
const Float _var[3] = {8.6556e+09, 1.8609e+10, 1.3224e+10};

/* Tabulated version of the Fourier Transforms of the normalized XYZ
 * Arrays are defined at the end of the file. For each profile, we
 * provide the real and imaginary part (with 'r' of 'i' prefix).
 */
#define nCMFs 512
extern const Float rShX[nCMFs];
extern const Float iShX[nCMFs];
extern const Float rShY[nCMFs];
extern const Float iShY[nCMFs];
extern const Float rShZ[nCMFs];
extern const Float iShZ[nCMFs];

/* Evaluate the FFT of XYZ sensitivity curves given an OPD and phase shift
*/
inline Spectrum evalSensitivity(Spectrum OPD, Spectrum shift, bool useGaussianFit)
{
      if (useGaussianFit)
      {
            // {Fitted version with phase shift}
            Spectrum phase = 2 * M_PI * OPD * 1.0e-9;
            Spectrum val, pos, var;
            val.fromLinearRGB(_val[0], _val[1], _val[2]);
            pos.fromLinearRGB(_pos[0], _pos[1], _pos[2]);
            var.fromLinearRGB(_var[0], _var[1], _var[2]);

            Spectrum xyz = val * sqrt(2 * M_PI * var) * cos(pos * phase + shift) *
                           exp(-phase * phase * var / 2);
            xyz[0] += 6.8922e-14 * std::sqrt(2 * M_PI * 9.0564e+09) *
                      std::cos(2.2399e+06 * phase[0] + shift[0]) *
                      std::exp(-9.0564e+09 * phase[0] * phase[0] / 2);
            return xyz / 1.0685e-7;
      }
      else
      {
            // {Tabulated version}
            Spectrum xyz;
            int idx;
            Spectrum mD = 2 * M_PI * OPD;
            Spectrum u = (mD / 30000.0) * (nCMFs - 1);

            // X Channel
            if (u[0] >= nCMFs)
            {
                  xyz[0] = 0.0;
            }
            else
            {
                  idx = floor(u[0]);
                  xyz[0] = std::cos(shift[0]) * rShX[idx] + std::sin(shift[0]) * iShX[idx];
            }

            // Y Channel
            if (u[1] >= nCMFs)
            {
                  xyz[1] = 0.0;
            }
            else
            {
                  idx = floor(u[1]);
                  xyz[1] = std::cos(shift[1]) * rShY[idx] + std::sin(shift[1]) * iShY[idx];
            }

            // Z Channel
            if (u[2] >= nCMFs)
            {
                  xyz[2] = 0.0;
            }
            else
            {
                  idx = floor(u[2]);
                  xyz[2] = std::cos(shift[2]) * rShZ[idx] + std::sin(shift[2]) * iShZ[idx];
            }

            return xyz;
      }
}

inline Spectrum evalSensitivityMean(int m, Spectrum tao, Spectrum shift, Float minHeight, Float maxHeight)
{
      Spectrum val, pos, var;
      val.fromLinearRGB(_val[0], _val[1], _val[2]);
      pos.fromLinearRGB(_pos[0], _pos[1], _pos[2]);
      var.fromLinearRGB(_var[0], _var[1], _var[2]);

      auto A = val * sqrt(2 * M_PI * var) / 1.0685e-7;      // == 1 for Y and Z
      auto B = 2 * M_PI * m * tao * pos;
      auto C = M_PI * m * tao;
      C *= 2 * C * var;
      auto CB2 = var / (2*sqr(pos));

      auto integrate = [&A, &B, &C, &CB2, &shift] (Float d) {
            auto sinv = sin(B*d + shift);
            auto cosv = cos(B*d + shift);
            auto Cd2 = C*sqr(d);
            auto cos_term = 2*CB2*d*cosv*(Cd2-6*CB2-Spectrum(1));
            auto sin_term = sinv*(Spectrum(1)+2*CB2+12*sqr(CB2)+0.5*sqr(Cd2)-Cd2-6*CB2*Cd2)/B;
            return A*(sin_term + cos_term);
      };

      auto res = integrate(maxHeight*1.0e-9) - integrate(minHeight*1.0e-9);

      // second part of x
      auto Ax = 6.8922e-14 * std::sqrt(2 * M_PI * 9.0564e+09) / 1.0685e-7;
      auto Bx = 2 * M_PI * m * tao[0] * 2.2399e+06;
      auto Cx = M_PI * m * tao[0];
      Cx *= 2 * Cx * 9.0564e+09;
      auto CB2x = 9.0564e+09 / (2*sqr(2.2399e+06));

      auto integrateX = [&Ax, &Bx, &Cx, &CB2x, &shift] (Float d) {
            auto sinv = std::sin(Bx*d + shift[0]);
            auto cosv = std::cos(Bx*d + shift[0]);
            auto Cd2 = Cx*sqr(d);
            auto cos_term = 2*CB2x*d*cosv*(Cd2-6*CB2x-1);
            auto sin_term = sinv*(1+2*CB2x+12*sqr(CB2x)+0.5*sqr(Cd2)-Cd2-6*CB2x*Cd2)/Bx;
            return Ax*(sin_term + cos_term);
      };

      res[0] += integrateX(maxHeight*1.0e-9) - integrateX(minHeight*1.0e-9);

      // Multiply P(d)
      res *= 1e9 / (maxHeight - minHeight);

      return res;
}

inline Spectrum evalSensitivitySquare(Spectrum tao, Spectrum shift, Float minHeight, Float maxHeight)
{
      // m == 1

      Spectrum val, pos, var;
      val.fromLinearRGB(_val[0], _val[1], _val[2]);
      pos.fromLinearRGB(_pos[0], _pos[1], _pos[2]);
      var.fromLinearRGB(_var[0], _var[1], _var[2]);

      auto A = 2 * M_PI * var * (val/1.0685e-7) * (val/1.0685e-7);
      auto B = 2 * M_PI * tao * pos;
      auto C = M_PI * tao;
      C *= 4 * C * var;
      auto CB2 = var / sqr(pos);

      auto integrate = [&A, &B, &C, &CB2, &shift] (Float d) {
            auto sinv = sin(2*(B*d + shift));
            auto cosv = cos(2*(B*d + shift));
            auto Cd2 = C*sqr(d);
            auto poly_term = Spectrum(d/2) - Cd2*d/6 + sqr(Cd2)*d/20 + shift/(2*B);
            auto cos_term = 0.25*CB2*d*cosv*(Cd2-1.5*CB2-Spectrum(1));
            auto sin_term = sinv*(Spectrum(1)+0.5*CB2+0.75*sqr(CB2)+0.5*sqr(Cd2)-Cd2-1.5*CB2*Cd2)/(4*B);
            return A*(poly_term + sin_term + cos_term);
      };

      auto res = integrate(maxHeight*1.0e-9) - integrate(minHeight*1.0e-9);

      // second part of x
      auto Ax = 2 * M_PI * 9.0564e+09 * (6.8922e-14/1.0685e-7) * (6.8922e-14/1.0685e-7);
      auto Bx = 2 * M_PI * tao[0] * 2.2399e+06;
      auto Cx = M_PI * tao[0];
      Cx *= 4 * Cx * 9.0564e+09;
      auto CB2x = 9.0564e+09 / sqr(2.2399e+06);

      auto integrateX1 = [&Ax, &Bx, &Cx, &CB2x, &shift] (Float d) {
            auto sinv = std::sin(2*(Bx*d + shift[0]));
            auto cosv = std::cos(2*(Bx*d + shift[0]));
            auto Cd2 = Cx*sqr(d);
            auto poly_term = d/2 - Cd2*d/6 + sqr(Cd2)*d/20 + shift[0]/(2*Bx);
            auto cos_term = 0.25*CB2x*d*cosv*(Cd2-1.5*CB2x-1);
            auto sin_term = sinv*(1+0.5*CB2x+0.75*sqr(CB2x)+0.5*sqr(Cd2)-Cd2-1.5*CB2x*Cd2)/(4*Bx);
            return Ax*(poly_term + sin_term + cos_term);
      };

      res[0] += integrateX1(maxHeight*1.0e-9) - integrateX1(minHeight*1.0e-9);

      // ignore cross term

      // Multiply P(d)
      res *= 1e9 / (maxHeight - minHeight);

      return res;
}

#endif

struct IridescenceParams
{
      Spectrum height;
      Spectrum eta1;
      Spectrum eta2;
      Spectrum eta3;
      Spectrum kappa3;
      Spectrum wavelengths;
      bool spectralAntialiasing;
      bool useGaussianFit;

      IridescenceParams(const Spectrum &height,
                        const Spectrum &eta1, const Spectrum &eta2, const Spectrum &eta3,
                        const Spectrum &kappa3, const Spectrum &wavelengths,
                        bool spectralAntialiasing = true, bool useGaussianFit = false) :
                        height(height),
                        eta1(eta1), eta2(eta2), eta3(eta3),
                        kappa3(kappa3), wavelengths(wavelengths),
                        spectralAntialiasing(spectralAntialiasing), useGaussianFit(useGaussianFit) {}
};

/* Our iridescence term accounting for the interference of light reflected
 * by the layered structure.
 */
inline Spectrum IridescenceTerm(Float ct1, const IridescenceParams& params)
{
      const Spectrum &height = params.height;
      const Spectrum &eta1 = params.eta1;
      const Spectrum &eta2 = params.eta2;
      const Spectrum &eta3 = params.eta3;
      const Spectrum &kappa3 = params.kappa3;
      const Spectrum &wavelengths = params.wavelengths;
      bool spectralAntialiasing = params.spectralAntialiasing;
      bool useGaussianFit = params.useGaussianFit;

      /* Compute the Spectral versions of the Fresnel reflectance and
    * transmitance for each interface. */
      Spectrum R12p, T121p, R23p, R12s, T121s, R23s, ct2;
      for (int i = 0; i < SPECTRUM_SAMPLES; ++i)
      {
            // Reflected and transmitted parts in the thin film
            // Note: This part needs to compute the new ray direction ct2[i]
            //       as ct2[i] is wavelength dependent.
            const Float scale = eta1[i] / eta2[i]; //(ct1 > 0) ? eta1[i]/eta2[i] : eta2[i]/eta1[i];
            const Float cosThetaTSqr = 1 - (1 - sqr(ct1)) * sqr(scale);

            /* Check for total internal reflection */
            if (cosThetaTSqr <= 0.0f)
            {
                  R12s[i] = 1.0;
                  R12p[i] = 1.0;

                  // Compute the transmission coefficients
                  T121p[i] = 0.0;
                  T121s[i] = 0.0;
            }
            else
            {
                  ct2[i] = std::sqrt(cosThetaTSqr);
                  fresnelConductorExact(ct1, eta2[i] / eta1[i], 0.0, R12p[i], R12s[i]);

                  // Reflected part by the base
                  fresnelConductorExact(ct2[i], eta3[i] / eta2[i], kappa3[i] / eta2[i], R23p[i], R23s[i]);

                  // Compute the transmission coefficients
                  T121p[i] = 1.0 - R12p[i];
                  T121s[i] = 1.0 - R12s[i];
            }
      }

      /* Optical Path Difference */
      const Spectrum D = 2.0 * eta2 * height[0] * ct2;
      const Spectrum Dphi = 2.0 * M_PI * D / wavelengths;

      /* Variables */
      Spectrum phi21p(0.), phi21s(0.), phi23p(0.), phi23s(0.),
          r123s, r123p, Rs, cosP, irid, I(0.);

      /* Evaluate the phase shift */
      fresnelPhaseExact(Spectrum(ct1), Spectrum(1.0), eta2, Spectrum(0.0), phi21p, phi21s);
      fresnelPhaseExact(ct2, eta2, eta3, kappa3, phi23p, phi23s);
      phi21p = Spectrum(M_PI) - phi21p;
      phi21s = Spectrum(M_PI) - phi21s;

      r123p = sqrt(R12p * R23p);
      r123s = sqrt(R12s * R23s);

#if SPECTRUM_SAMPLES == 3
      if (spectralAntialiasing)
      {

            Spectrum C0, Cm, Sm;
            const Spectrum S0 = Spectrum(1.0);

            /* Iridescence term using spectral antialiasing for Parallel polarization */
            // Reflectance term for m=0 (DC term amplitude)
            Rs = (sqr(T121p) * R23p) / (Spectrum(1.0) - R12p * R23p);
            C0 = R12p + Rs;
            I += C0 * S0;

            // Reflectance term for m>0 (pairs of diracs)
            Cm = Rs - T121p;
            for (int m = 1; m <= 2; ++m)
            {
                  Cm *= r123p;
                  Sm = 2.0 * evalSensitivity(m * D, m * (phi23p + phi21p), useGaussianFit);
                  I += Cm * Sm;
            }

            /* Iridescence term using spectral antialiasing for Perpendicular polarization */
            // Reflectance term for m=0 (DC term amplitude)
            Rs = (sqr(T121s) * R23s) / (Spectrum(1.0) - R12s * R23s);
            C0 = R12s + Rs;
            I += C0 * S0;

            // Reflectance term for m>0 (pairs of diracs)
            Cm = Rs - T121s;
            for (int m = 1; m <= 2; ++m)
            {
                  Cm *= r123s;
                  Sm = 2.0 * evalSensitivity(m * D, m * (phi23s + phi21s), useGaussianFit);
                  I += Cm * Sm;
            }

            // Ensure that the BRDF is non negative and convert it to RGB. Mitsuba uses
            // a XYZ -> RGB convertion that does not preserve white spectra. Instead, we
            // use the CIE RGB and CIE XYZ 1931 conversion:
            //
            //         source: https://en.wikipedia.org/wiki/CIE_1931_color_space
            //
            //*
            const Float r = 2.3646381 * I[0] - 0.8965361 * I[1] - 0.4680737 * I[2];
            const Float g = -0.5151664 * I[0] + 1.4264000 * I[1] + 0.0887608 * I[2];
            const Float b = 0.0052037 * I[0] - 0.0144081 * I[1] + 1.0092106 * I[2];
            I[0] = r;
            I[1] = g;
            I[2] = b;

            I.clampNegative();
      }
      else
      {
#endif
            /* Iridescence term using Airy summation (Eq. 11) for Parallel polarization */
            Rs = (sqr(T121p) * R23p) / (Spectrum(1.0) - R12p * R23p);
            cosP = cos(Dphi + phi23p + phi21p);
            irid = (r123p * cosP - sqr(r123p)) / (Spectrum(1.0) - 2.0 * r123p * cosP + sqr(r123p));
            I = R12p + Rs + 2.0 * (Rs - T121p) * irid;

            /* Iridescence term using Airy summation (Eq. 11) for Perpendicular polarization */
            Rs = (sqr(T121s) * R23s) / (Spectrum(1.0) - R12s * R23s);
            cosP = cos(Dphi + phi23s + phi21s);
            irid = (r123s * cosP - sqr(r123s)) / (Spectrum(1.0) - 2.0 * r123s * cosP + sqr(r123s));
            I += R12s + Rs + 2.0 * (Rs - T121s) * irid;

            // Assure that the BRDF is non negative
            I.clampNegative();
#if SPECTRUM_SAMPLES == 3
      }
#endif

      return 0.5 * I;
}

inline std::tuple<Spectrum, Spectrum> IridescenceMean(Float ct1, const IridescenceParams& params,
                                                      Float minHeight, Float maxHeight)
{
      const Spectrum &height = params.height;
      const Spectrum &eta1 = params.eta1;
      const Spectrum &eta2 = params.eta2;
      const Spectrum &eta3 = params.eta3;
      const Spectrum &kappa3 = params.kappa3;
      const Spectrum &wavelengths = params.wavelengths;
      bool spectralAntialiasing = params.spectralAntialiasing;
      bool useGaussianFit = params.useGaussianFit;

      /* Compute the Spectral versions of the Fresnel reflectance and
       * transmitance for each interface. */
      Spectrum R12p, T121p, R23p, R12s, T121s, R23s, ct2;
      for (int i = 0; i < SPECTRUM_SAMPLES; ++i)
      {
            const Float scale = eta1[i] / eta2[i];
            const Float cosThetaTSqr = 1 - (1 - sqr(ct1)) * sqr(scale);

            /* Check for total internal reflection */
            if (cosThetaTSqr <= 0.0f)
            {
                  R12s[i] = 1.0;
                  R12p[i] = 1.0;
                  T121p[i] = 0.0;
                  T121s[i] = 0.0;
            }
            else
            {
                  ct2[i] = std::sqrt(cosThetaTSqr);
                  fresnelConductorExact(ct1, eta2[i] / eta1[i], 0.0, R12p[i], R12s[i]);
                  fresnelConductorExact(ct2[i], eta3[i] / eta2[i], kappa3[i] / eta2[i], R23p[i], R23s[i]);
                  T121p[i] = 1.0 - R12p[i];
                  T121s[i] = 1.0 - R12s[i];
            }
      }

      /* Variables */
      Spectrum phi21p(0.), phi21s(0.), phi23p(0.), phi23s(0.),
          r123s, r123p, Rs, cosP, irid, I(0.), V(0.);

      /* Evaluate the phase shift */
      fresnelPhaseExact(Spectrum(ct1), Spectrum(1.0), eta2, Spectrum(0.0), phi21p, phi21s);
      fresnelPhaseExact(ct2, eta2, eta3, kappa3, phi23p, phi23s);
      phi21p = Spectrum(M_PI) - phi21p;
      phi21s = Spectrum(M_PI) - phi21s;

      r123p = sqrt(R12p * R23p);
      r123s = sqrt(R12s * R23s);

#if SPECTRUM_SAMPLES == 3
      if (spectralAntialiasing)
      {
            Spectrum C0, C1, S1;
            const Spectrum S0 = Spectrum(1.0);

            auto tao = 2.0 * eta2 * ct2;

            /* Iridescence term using spectral antialiasing for Parallel polarization */
            // Reflectance term for m=0 (DC term amplitude)
            Rs = (sqr(T121p) * R23p) / (Spectrum(1.0) - R12p * R23p);
            C0 = R12p + Rs;
            I += C0 * S0;

            // Reflectance term for m>0 (pairs of diracs)
            C1 = (Rs - T121p) * r123p;
            auto mean = evalSensitivityMean(1, tao, (phi23p + phi21p), minHeight, maxHeight);
            S1 = 2.0 * mean;
            I += C1 * S1;

            // Variance
            auto meanSquare = evalSensitivitySquare(tao, (phi23p + phi21p), minHeight, maxHeight);
            V += 4*C1*C1*(meanSquare - mean*mean);

            /* Iridescence term using spectral antialiasing for Perpendicular polarization */
            // Reflectance term for m=0 (DC term amplitude)
            Rs = (sqr(T121s) * R23s) / (Spectrum(1.0) - R12s * R23s);
            C0 = R12s + Rs;
            I += C0 * S0;

            // Reflectance term for m>0 (pairs of diracs)
            C1 = (Rs - T121s) * r123s;
            mean = evalSensitivityMean(1, tao, (phi23s + phi21s), minHeight, maxHeight);
            S1 = 2.0 * mean;
            I += C1 * S1;

            // Variance
            meanSquare = evalSensitivitySquare(tao, (phi23s + phi21s), minHeight, maxHeight);
            V += 4*C1*C1*(meanSquare - mean*mean);

            I.clampNegative();
            V.clampNegative();
      }
      else
      {
#endif
            // TODO
#if SPECTRUM_SAMPLES == 3
      }
#endif

      return std::make_tuple(0.5*I, 0.5*V);
}

/* Static data: define the tabulated values for the Fourier Transform of
 * the XYZ profiles. The sampling of the CMFs can be adjusted and made non
 * linear to favour high energy regions.
 */
const Float rShX[] = {
    1, 0.9940450157203137, 0.9772224263826175, 0.9497385496264387, 0.9119207996601364, 0.8642135529984489, 0.80717264318441, 0.7414585554565463, 0.6678284075551504, 0.587126817015461, 0.5002757682193087, 0.4082636040312175, 0.3121332769051595, 0.2129700028041266, 0.1118523353186993, 0.01011393829168318, -0.09106300768793081, -0.1905649434248358, -0.2873071559426191, -0.3802460007414552, -0.4683905087801271, -0.5508132467840576, -0.6266603098893443, -0.6951603373480476, -0.7556324548985469, -0.8074930612927199, -0.8502613912058101, -0.8832576820935848, -0.905953624434137, -0.9187724676150826, -0.921709569557586, -0.9148697272002244, -0.8984640621179378, -0.8728057267158251, -0.838304505352824, -0.7954603965808853, -0.7448562732738022, -0.6871497266666596, -0.6230642081474611, -0.5533795889678754, -0.4789109721939761, -0.4002953837927593, -0.3188335304506267, -0.2354293386916529, -0.1509868786693581, -0.06640038508079547, 0.01745532161108804, 0.09973391163065114, 0.1796260271449828, 0.2563670854772856, 0.3292443322116229, 0.3976030835040103, 0.4608521087526135, 0.5184681169242846, 0.5695904289751799, 0.614037955484585, 0.6516892441687734, 0.6823433789180652, 0.7058751084255925, 0.7222333560388162, 0.731438970595242, 0.7335817795926693, 0.7288170122798454, 0.7173611653702741, 0.6994873880491244, 0.675520465743993, 0.6458314837611751, 0.6106673423820206, 0.5704456173338581, 0.5259351022464303, 0.4776396371904227, 0.4260780903990151, 0.3717789252073649, 0.3152750103542828, 0.2570987199832807, 0.1977773624211835, 0.1378289693473261, 0.07775846940844101, 0.01805426279558881, -0.04081479411111402, -0.09839168173097784, -0.154080258752472, -0.2075990020952878, -0.2585885644372103, -0.3067184940609013, -0.3516877919821043, -0.393225204672231, -0.431089287565006, -0.465068275361727, -0.4949797951906427, -0.5206704579592919, -0.5420153618003649, -0.5589175393969866, -0.5713073782363104, -0.5788415048447443, -0.5817035960808339, -0.5800135123377278, -0.5738134348660033, -0.5631702133601519, -0.548175052161749, -0.528943246598566, -0.5056139580089486, -0.4783500120377696, -0.4473377012452166, -0.4127865700085532, -0.3749291571783975, -0.3340206700266061, -0.290247885309481, -0.2439215397652292, -0.1955089893122244, -0.1453563034332072, -0.09382784008514605, -0.04130474765535064, 0.01181679401117986, 0.06512760437666276, 0.118207810804339, 0.1706294358346956, 0.2219592585378614, 0.2717619396833859, 0.3196033947100501, 0.3650290908601219, 0.4073739209390264, 0.4464256129154297, 0.4818003347251554, 0.5131371332305563, 0.540102096366093, 0.5623924464916041, 0.5797405086258389, 0.5919174961339159, 0.5987370561660189, 0.6000585177123294, 0.5957897865622104, 0.5858898337351707, 0.5703707270781356, 0.5489282423160149, 0.5219834294259184, 0.4898409697698257, 0.4527521807708229, 0.411022312730395, 0.3650086193227926, 0.3151177849015706, 0.2618027205039055, 0.205558748889499, 0.1469192073230383, 0.08645050499685218, 0.0247466798893002, -0.03757649263803533, -0.09985788562337768, -0.1613613718163499, -0.2214489025319487, -0.2794889351752338, -0.3348654521600775, -0.3869849799474913, -0.4352834523855242, -0.4792328336231982, -0.5183474176457381, -0.5521897242876594, -0.580375915398921, -0.6025806596319727, -0.6185413800378575, -0.6279841470455865, -0.6303524983805743, -0.6261066353799146, -0.6152865290915355, -0.5980045885930976, -0.5744446734566627, -0.5448602696171849, -0.5095718463553141, -0.4689634220092846, -0.4234783756317745, -0.3736145509970382, -0.3199187080371331, -0.2629803848452415, -0.2034252407349984, -0.1418244671421427, -0.07903966320724665, -0.01579357970042743, 0.04722067560573527, 0.1093158195983951, 0.1698182326402622, 0.228075608151158, 0.2834643230637407, 0.3353964429304845, 0.3833262801163121, 0.4267564291006053, 0.4652432093685429, 0.4984014536215302, 0.5256521317632625, 0.546703101621166, 0.5616440456331906, 0.5703797061525724, 0.572885538681438, 0.5692071941799961, 0.5594591870658885, 0.5438227738731551, 0.5225430765864634, 0.4959254932174385, 0.4643314461574845, 0.4281735261435133, 0.3879100962346896, 0.3439949282235598, 0.2968466133944477, 0.2472951532964469, 0.1959228109156719, 0.1433218053872979, 0.09008735633622054, 0.03681082374745498, -0.01592697732912523, -0.06756221147269496, -0.1175541355710993, -0.1653908231822407, -0.2105944400689915, -0.2527260168075847, -0.2913896713108883, -0.3259258979993139, -0.3562474671092355, -0.382176964187641, -0.4035385341611737, -0.4202110135921436, -0.4321278679705688, -0.4392764957000309, -0.4416969240885015, -0.4394799292563241, -0.432764617937844, -0.4217355146247949, -0.4066192023289221, -0.3876805693869864, -0.3650736352184952, -0.3391930618931658, -0.3105470068878832, -0.2795140689062947, -0.2464861956936646, -0.2118638673266235, -0.1760513563699503, -0.1394521186707035, -0.1024643650770962, -0.0654768603920465, -0.02886499145914754, 0.007012858505306671, 0.04181859731076149, 0.07520710402821219, 0.1067695189412498, 0.1363524122961763, 0.1637342203454024, 0.1887245766790503, 0.211164936536629, 0.2309288086635791, 0.2479216113713476, 0.2620801734284563, 0.2733719039490914, 0.281793658527543, 0.2873703314693913, 0.2901532060775718, 0.2902180965530518, 0.2874712386715068, 0.2822225019946816, 0.2746413065705643, 0.2648836660349568, 0.2531178132771449, 0.2395218651781889, 0.2242815398268889, 0.2075879513493583, 0.189635504742693, 0.1706199101859361, 0.1507363332589153, 0.1301776943779931, 0.1091331276034999, 0.08778173910165689, 0.06632743421474957, 0.0449491792599129, 0.02380591791667974, 0.003047432475124512, -0.0171861059495994, -0.03676445077189201, -0.05556746931921007, -0.0734852217623307, -0.09041794385829538, -0.106275948211559, -0.120979458895727, -0.1344583941218846, -0.1466243607896111, -0.1573580356155605, -0.1667065637003596, -0.1746385327801716, -0.1811307123240239, -0.1861678105834717, -0.1897422596150171, -0.1918540311219773, -0.1925104842242397, -0.1917262445726234, -0.1895231126083933, -0.1859299972553591, -0.1809828699473228, -0.1747247326608464, -0.1671095924853419, -0.1582934105461101, -0.1483520692826716, -0.1373585193883645, -0.1253924194620277, -0.1125399703396845, -0.0988936943380664, -0.08455215202964761, -0.06961959018414435, -0.05420551569128178, -0.0384241916041237, -0.02239405288942865, -0.006237041017877361, 0.009914633926088367, 0.0259169710615275, 0.04164254943388465, 0.05696419361229037, 0.0717562126072727, 0.08589549549971742, 0.09926263561587931, 0.1117430702782357, 0.123228222304165, 0.1336166287648992, 0.1428150420675123, 0.1507394881918723, 0.1573162669086508, 0.1624482495657187, 0.1660206361400796, 0.1680913576593078, 0.1686406676851687, 0.1676628384185918, 0.1651664196885201, 0.1611743481421558, 0.1557239018982277, 0.148866497863769, 0.1406673309145774, 0.1312048561743105, 0.1205701176709137, 0.1088659286755651, 0.09620591101252786, 0.08265710662428319, 0.0684309083304553, 0.05367192076449746, 0.03852583310613839, 0.02314184558296361, 0.007671102144385855, -0.007734902319463648, -0.02292592149558805, -0.03775454506617615, -0.05207775530728299, -0.06575843449871671, -0.0786668053542611, -0.09068178748107422, -0.1016307184403159, -0.1114246713163246, -0.120013570237251, -0.1273270108292745, -0.1333084920758517, -0.1379158682366723, -0.1411216352189106, -0.1429130498243814, -0.1432920823281884, -0.1422752048471245, -0.1398930199064656, -0.1361897354894225, -0.1312224946330756, -0.125031247222586, -0.1176694186978399, -0.1093040715233986, -0.1000370905132877, -0.08997751755471282, -0.07924023770560344, -0.06794462985231958, -0.0562131984340879, -0.04417020258487164, -0.03194029869302458, -0.01964721184233579, -0.007412450884982232, 0.004645918980828325, 0.01641443328474094, 0.02774067246775069, 0.03854975996065596, 0.04875162402598104, 0.05826402447490616, 0.06701388525047562, 0.07493774954726382, 0.08198211394955987, 0.08810364226004586, 0.09326926113120768, 0.09745614097643172, 0.100651566912532, 0.1028527056572934, 0.1040662753628179, 0.104253502683029, 0.1034704948107789, 0.1017815231868133, 0.09922854419133909, 0.09585990300912811, 0.09172959789872018, 0.08689652380869897, 0.08142370519853154, 0.07537752750486644, 0.0688269761715349, 0.0618428915439613, 0.05449724722813724, 0.0468624587432479, 0.03900669230131588, 0.03100631261744332, 0.02294328250604689, 0.01488482197130333, 0.006895590881326833, -0.0009626902007970829, -0.008631566894789519, -0.01605612668574975, -0.02318522516034824, -0.02997166512286821, -0.03637233141393236, -0.04234828466668295, -0.04786481755974646, -0.052890910152942, -0.05735365813768503, -0.06127514030192378, -0.06463882597471193, -0.06743222679118163, -0.06964680399463367, -0.07127786732197962, -0.07232446801883283, -0.07278928811191521, -0.07267852762636805, -0.07200179098353261, -0.07077197336030887, -0.06900514734363074, -0.06672044978197611, -0.06391154739804047, -0.06062367526001065, -0.05689881682839509, -0.05276797168107501, -0.04826454138910108, -0.04342418736760212, -0.03828467745425625, -0.0328857193711385, -0.0272687793652273, -0.02147688452268086, -0.01555440750852847, -0.009546832792206328, -0.003500503775119075, 0.002535645648454881, 0.008506711843627328, 0.0143661630682418, 0.02006742430136935, 0.02556477731417328, 0.03081372088437002, 0.03577133830300099, 0.04039666863175954, 0.04465107782721101, 0.04849862555747343, 0.05190642330322, 0.05484497916339857, 0.05728852468129091, 0.05921315080264011, 0.0605572719568326, 0.06135308090058447, 0.06159404773654277, 0.06127861100736932, 0.06041026764617031, 0.05899761337446189, 0.05705433146203878, 0.05459912837212554, 0.05165561546024773, 0.04825213656809472, 0.04442154204561539, 0.04020091043668472, 0.03563121976680716, 0.03074108967058573, 0.02559642326266332, 0.02025544395887706, 0.01477090882640944, 0.009196917804560629, 0.003588353053235289, -0.001999694344503621, -0.007512503678985425, -0.01289630311832312, -0.01809884057056305, -0.02306994008152143, -0.02776203706131781, -0.0321306858340019, -0.03612065907022888, -0.03968384557518716, -0.04280870593800514, -0.04546924679539362, -0.04764459662213322, -0.04931918961502475, -0.05048288765576057, -0.05113103919332472, -0.05126447467292062, -0.05088943892644827, -0.05001746171968502, -0.04866516841413924, -0.04685403343784574, -0.04460728530678813, -0.0419291299109855, -0.03888984530958663, -0.03552767361973286, -0.03188351177982434, -0.02800040381375384, -0.02392301770744531, -0.01969711355286261, -0.01536900961641498, -0.01098505290657599, -0.006591100651658522, -0.002232018855971855, 0.00204879621517387, 0.006209868159380842, 0.01019862840878762, 0.01398226595757954, 0.01753197254038925, 0.02081892280301346, 0.02381794863981729};
const Float iShX[] = {
    0, -0.1039265251057171, -0.2065800881652519, -0.3068169175447939, -0.4035232749710411, -0.4956283326432087, -0.5821165010554308, -0.6620390621357937, -0.7345249712176964, -0.7987907018246247, -0.8541490191476078, -0.9000165812883885, -0.935920281680925, -0.9615022614283765, -0.9758846187690258, -0.9792909095236924, -0.9720643738512703, -0.9543673358128768, -0.9264788514596765, -0.8887903238279093, -0.841799844223976, -0.7861053416458816, -0.722396635496359, -0.6514464987902822, -0.5741008497226986, -0.491268198636791, -0.4039084850217542, -0.3129472285593854, -0.2194887288786206, -0.124775533430921, -0.02984930145713428, 0.06426227655387436, 0.1565569015393633, 0.2460685754330408, 0.3318778779226862, 0.4131215006274416, 0.4890009399901625, 0.5587902622211787, 0.6218428664965197, 0.6775971861479539, 0.7255571230881924, 0.7646437891066055, 0.7952680332396781, 0.8172736469902422, 0.8306000304446685, 0.8352803582041592, 0.8314387228924249, 0.8192863209640207, 0.7991167553302759, 0.7713005379353173, 0.7362788827800911, 0.694556885948524, 0.6466961938939703, 0.5933072645661417, 0.5347853132845889, 0.4720841711165213, 0.4060392413680439, 0.3373748291071983, 0.2668193034649554, 0.1950973904115439, 0.1229228339475449, 0.05099149699177323, -0.02002503547406873, -0.08948529765230037, -0.1567831847799329, -0.2213525615825705, -0.2826712132104462, -0.3401198092475141, -0.3931424399938337, -0.4415845268436324, -0.4851428145777457, -0.5235674933413822, -0.5566618781769419, -0.5842815660899802, -0.6063331185311567, -0.6227723207636833, -0.6336020722130135, -0.6388699635615807, -0.6386655970663906, -0.6331177063629009, -0.6223618361654154, -0.606240729651279, -0.5854172951567668, -0.5601529631378043, -0.5307313270691141, -0.4974552173246956, -0.4606439123543703, -0.4206305067861361, -0.3777594503301892, -0.3323842656826388, -0.2848654481187974, -0.2355685442169461, -0.1848624022478914, -0.1331175822820824, -0.08068937626730259, -0.02801968565189546, 0.02451076146624515, 0.0765396343014686, 0.1277096289737798, 0.1776695520338551, 0.2260754255670441, 0.2725916361145054, 0.3168921471578391, 0.3586617918695834, 0.397597659295593, 0.4334105831725086, 0.4658267382665237, 0.494443025856113, 0.518938797835423, 0.5392911553906423, 0.5553140895926627, 0.5668473577214119, 0.5737585404579088, 0.5759450735406395, 0.5733362185606445, 0.5658949351549878, 0.5536196150824503, 0.5365456375717161, 0.5147467049547395, 0.4883359179614676, 0.4574289743247147, 0.4219465051093571, 0.3824901014419413, 0.339347140927066, 0.2928438166495982, 0.2433435893043553, 0.1912451658574032, 0.13698000245967, 0.08100933617149957, 0.02382075707339, -0.03407566056131679, -0.09215164226863169, -0.1498653798313914, -0.2066665406452572, -0.2618565462451132, -0.3148951547170942, -0.3652739397790236, -0.4124716978075438, -0.4559898510728575, -0.4953582354310223, -0.5301407327754077, -0.5599406748937066, -0.5844059462872755, -0.6032337153828805, -0.6161747263944892, -0.6230370878553483, -0.6236894985085464, -0.6178113436226361, -0.605362371441044, -0.5867290542138515, -0.5620628422118088, -0.5315810030135164, -0.4955650675080551, -0.4543585107919907, -0.4083636840824629, -0.3580380231914673, -0.3038895683215953, -0.2464718388412832, -0.1863781151715681, -0.1242351878634685, -0.06068899357484055, 0.003553701913867125, 0.06769991787386895, 0.1310618762362039, 0.1929586804285803, 0.2527240518618398, 0.3097139329987797, 0.3633138658769553, 0.4129460572928231, 0.4580760452480311, 0.4982188856725362, 0.5329447838250672, 0.5618841010716017, 0.5847316748902006, 0.6007838895019431, 0.6102202417844304, 0.6130765042351221, 0.6093567758484228, 0.5991386325952303, 0.5825719379825882, 0.5598768138531263, 0.5313407985991097, 0.4973152294992407, 0.4582108949504631, 0.4144930108636701, 0.3666755833297232, 0.3153152267551851, 0.2609078633944011, 0.2041640338823222, 0.1458624490628295, 0.08666046650271714, 0.02721813554638386, -0.03180940341031013, -0.08977937624072471, -0.1460686024703748, -0.2000804098844442, -0.2512511581351344, -0.2990562985947227, -0.3430159039599755, -0.3826996080870612, -0.4176570422780628, -0.4472477700521048, -0.4715757132921549, -0.4904653036314112, -0.5038048852430721, -0.5115468139576667, -0.5137068077199443, -0.5103625701449259, -0.5016517171326531, -0.4877690442066163, -0.4689631793752672, -0.4455326728096056, -0.4178215804200299, -0.3862146034477533, -0.3509123866145928, -0.3126297150314452, -0.2718927354154068, -0.2291958073507898, -0.1850425471038233, -0.1399398826336042, -0.09439223037122518, -0.04889585953906431, -0.003933505827245167, 0.04003070834159386, 0.0825559948801399, 0.1232292160063061, 0.1616687547555863, 0.1973927079380603, 0.2300660613933512, 0.2595429789541496, 0.2856107093934108, 0.3080998645024339, 0.3268847908180463, 0.3418834230185853, 0.3530566397071842, 0.3604071476257875, 0.3639779251805091, 0.3638502604670185, 0.3601414227279392, 0.3530020093240032, 0.3425595767108683, 0.3288905510193589, 0.3124625910007948, 0.2935329243857703, 0.2723738653805693, 0.2492692283262223, 0.224510785382862, 0.1983948089859366, 0.1712187368274344, 0.143277993767607, 0.114863001434127, 0.08625640237131495, 0.0577305215183043, 0.02954508357878966, 0.002011284618971753, -0.02466174548310762, -0.05027570559450584, -0.0746450991687386, -0.09760441264021352, -0.119008542106138, -0.1387329675708805, -0.1566736907923878, -0.1727469550241789, -0.1868887668177279, -0.1990542415292142, -0.2092167952572792, -0.2173672066248627, -0.2234154311256872, -0.2274094224473073, -0.2294679875198243, -0.2296436130094213, -0.2279997437192199, -0.2246095714408544, -0.2195548578689778, -0.2129248048504279, -0.204814983036384, -0.1953263277482088, -0.1845642085934312, -0.1726375771142166, -0.1596581945519677, -0.1457231935388141, -0.130923866874206, -0.1154400373732613, -0.09938850529019183, -0.0828859964065122, -0.06604882953463898, -0.04899261899816969, -0.03183200219472587, -0.01468038233303557, 0.002350323335580487, 0.01914993901078943, 0.03561029810294321, 0.05162553092037431, 0.06709237888410943, 0.08184910750068521, 0.09583725574672307, 0.1089701595114357, 0.1211605173415831, 0.1323260173695737, 0.1423898889660335, 0.1512814800629161, 0.1589368531475024, 0.1652993917009507, 0.1703204077761807, 0.1739597404977573, 0.1761863345376893, 0.1769787870897273, 0.1762506273988877, 0.1740262078664946, 0.1703718717937829, 0.1653129695645401, 0.1588863589412387, 0.1511404406652313, 0.1421350804438272, 0.1319414115832799, 0.1206415141141488, 0.1083279679505394, 0.09510327940345698, 0.08107918220584799, 0.0663758160771728, 0.05111132148217794, 0.0354204983939171, 0.01947932767094064, 0.003431710542162042, -0.01257643678870651, -0.02839859686841653, -0.04388910047896586, -0.05890459515878611, -0.07330551546491507, -0.08695753733532985, -0.09973299879736178, -0.1115122693898487, -0.1221850510233273, -0.1316515935914515, -0.1397085235342838, -0.1463782483611373, -0.1516097908563745, -0.1553603904148464, -0.157602230237365, -0.1583227348838655, -0.1575246987737026, -0.1552262436051728, -0.151460604770373, -0.1462757489434419, -0.1397338270947354, -0.1319104692054737, -0.1228939289027118, -0.1127310312598613, -0.1015722935692858, -0.08957592861106894, -0.07687123077388056, -0.06359326809282866, -0.0498813739612102, -0.03587760713943419, -0.02172519839946329, -0.00756700210063695, 0.006456029271196332, 0.02020633004910886, 0.03355114895060032, 0.04636390328406547, 0.05850485626209335, 0.06982388049145603, 0.08026388886763058, 0.08973832619845404, 0.09817225615015748, 0.1055029095881887, 0.1116800851254743, 0.1166664006506898, 0.120437396426549, 0.1229814921175281, 0.124299801808147, 0.1244058126882059, 0.1233249345927645, 0.1210939289760358, 0.1176709817609913, 0.1132139204236313, 0.107793566749829, 0.1014862806001392, 0.09437497013495638, 0.08654802579006807, 0.07809823260782234, 0.0691216743225042, 0.05971664220242291, 0.04998256110662756, 0.04001894453210059, 0.02992438961991639, 0.01979562216965206, 0.00973609470779495, -0.0001547982604725594, -0.009794697656072661, -0.01910454990323682, -0.02801112395763952, -0.03644745725476572, -0.04435321428071813, -0.0516749585889676, -0.05836634014183888, -0.06438820083320732, -0.06970860193911409, -0.07430277803744366, -0.07815302262931041, -0.08122609855334124, -0.08349614121218284, -0.08501162909740372, -0.08578253991223583, -0.08582433766433614, -0.08515757267961266, -0.08380746562034189, -0.0818034816326523, -0.07917890032617164, -0.07597038680372473, -0.07221756842198847, -0.06796262138593454, -0.06324987067178832, -0.05812501266141471, -0.05260935051449735, -0.04678757363118992, -0.04070905015483542, -0.03442344580497218, -0.02798045524301572, -0.02142955261613071, -0.01481975979949499, -0.008199430590513668, -0.001616048912249777, 0.004883961041589292, 0.01125541484148888, 0.01745454014184372, 0.02343914330654685, 0.02915052101368197, 0.0345543165226494, 0.03962182036807972, 0.04431957690228185, 0.04861659236151323, 0.05248449761584465, 0.0558977111817703, 0.05883360148182183, 0.061272646943236, 0.06319859215854119, 0.06459859799257747, 0.0654633832202228, 0.06578735502370538, 0.06555118747287952, 0.06474536418105958, 0.06340756420288027, 0.06154923304174559, 0.05918587201096314, 0.05633701509633693, 0.05302617185087854, 0.04928073433986642, 0.04513184656591998, 0.04061423526360632, 0.03576600145499627, 0.03062837269457226, 0.02524541649641339, 0.0196631293639768, 0.01392377097641206, 0.00809499646138292, 0.002228847013059287, -0.003621986415868009, -0.009404648710656164, -0.01506662012488621, -0.02055622439478808, -0.02582313979976917, -0.03081890731498076, -0.03549742986074143, -0.0398154565911541, -0.04373304618254162, -0.04721400318260005, -0.05019433981605383, -0.05266273402396591, -0.05460963932990846, -0.05601922702996789, -0.05688105268639888, -0.05719018049347527, -0.05694724754595282, -0.05615846665248399, -0.05483556708900784, -0.05299567345403915, -0.05066112355943862, -0.04785922705706767, -0.04462196725416225, -0.04097313874198653, -0.03695285775972873, -0.03262676117201262, -0.02804220667578243, -0.02324874670413859, -0.01829756929124106, -0.01324092385452967, -0.008131538848673316, -0.003022038318819222, 0.002035635634265798, 0.006990787567166966, 0.01179451386358603, 0.01640022191191123, 0.02076211297950676, 0.0248146158509399, 0.02854181291441787, 0.03191224998723572, 0.03489889434182123, 0.03747934748553171, 0.03963599974741036, 0.04135612582249755, 0.04263192117923556, 0.04346047998067694, 0.04384371589667994, 0.04378822788345482, 0.04330511367047119, 0.04240973431505356, 0.04109700574245626, 0.0394111419945488, 0.03738773503600762, 0.03505670199953677, 0.03245025513992951};
const Float rShY[] = {
    1, 0.9939681908588384, 0.9769257745415467, 0.9490742060664386, 0.91073346660262, 0.8623383958384605, 0.8044337971112357, 0.7376683692039616, 0.662787530553207, 0.5806252127809923, 0.4920947108665695, 0.3981786868176719, 0.2999184323004336, 0.1984025032647205, 0.09470714318118786, -0.009807712592017123, -0.1139277827358336, -0.2165053074309497, -0.3164129885381666, -0.4125561788508392, -0.503884679247279, -0.5894040213464216, -0.6681861189683883, -0.7393791785080135, -0.8022167661786055, -0.8560259389019303, -0.9002343553278552, -0.9340481077187019, -0.9568190270530976, -0.9689193179840726, -0.9702654865214695, -0.9608921845283125, -0.9409512294691103, -0.9107093765803181, -0.8705448737726208, -0.8209428417401743, -0.7624895334337329, -0.6958655381512304, -0.6218380059147777, -0.5412519774469094, -0.4550074075946234, -0.3638020799930355, -0.2691484502742754, -0.1721080349091897, -0.07376134225683421, 0.02480394211421128, 0.1225047612411296, 0.2182743606543135, 0.3110737095684418, 0.3999025520468554, 0.4838099753506594, 0.5619043882798729, 0.6333628089078185, 0.6974393686392223, 0.752927277813553, 0.7995076500563965, 0.8369469995829014, 0.8649088871235996, 0.8831627174457297, 0.8915853181685034, 0.8901613650213676, 0.8789826595827657, 0.8582462770103874, 0.8282516124727404, 0.7893963658188595, 0.742171514385679, 0.687155333652361, 0.6247902958231671, 0.5557934832529084, 0.4813613102299619, 0.4023575571204762, 0.319686924285144, 0.2342849154009426, 0.1471075023014966, 0.05912068040383164, -0.02870997632993097, -0.1154296531006083, -0.200104419019916, -0.2818310364168947, -0.3597463563160726, -0.4330039837089845, -0.5003600759704064, -0.5615272355039118, -0.6159110037169275, -0.6629969020905294, -0.7023549703209631, -0.7336433617143232, -0.7566109707593651, -0.7710990777290672, -0.7770420050999632, -0.7744667904427898, -0.7634918901435417, -0.7443249377777027, -0.7172595901021939, -0.6822124992215217, -0.6400298630462042, -0.591425030469501, -0.5370042813006268, -0.4774284142178543, -0.4134050182066673, -0.3456803602080031, -0.2750309769302536, -0.2022550604629843, -0.1281637281527514, -0.05357226716619741, 0.02070855671588333, 0.09388103823929544, 0.1650670278944715, 0.2333860188922082, 0.2982401292193746, 0.3589808721585189, 0.4150129684988107, 0.4657998447823488, 0.5108684406664096, 0.5498132872480502, 0.5822998259093923, 0.6080669451156233, 0.626928720564438, 0.6387753520756017, 0.6435732985537002, 0.6413037282392421, 0.6316196808934106, 0.6152880995092679, 0.5925865691530664, 0.5638559462342667, 0.5294957716417007, 0.4899591534419357, 0.4457471793263933, 0.3974029225478323, 0.3455051079953156, 0.2906615073061324, 0.2335021334944855, 0.1746723064864651, 0.1148256611901814, 0.05464440191391343, -0.005132878568965389, -0.0638696952669003, -0.1209574090748379, -0.1758150734452499, -0.2278950266663034, -0.2766880470070707, -0.3217280267783136, -0.3625961267465546, -0.3989243779872491, -0.4303987041182856, -0.4567613428654042, -0.4778126520237994, -0.4931894485016712, -0.5027675070082548, -0.5068070814568441, -0.5053647124855335, -0.4985536515599789, -0.4865416530875731, -0.4695482504980824, -0.4478415527761659, -0.4217346018556176, -0.3915813347734665, -0.3577721975130545, -0.3207294600211571, -0.2809022839508172, -0.2387406600734582, -0.1946723081756267, -0.1493740767262194, -0.1033495252345427, -0.05710014263737376, -0.01112018605979023, 0.03410830761257411, 0.0781202912333186, 0.1204722148105261, 0.1607462309738862, 0.1985540622233709, 0.2335404975476471, 0.2653864900879974, 0.2938118318033506, 0.3182992126900088, 0.3388416414345541, 0.3553625770297736, 0.3677693729739466, 0.3760144487612307, 0.3800948740433573, 0.3800515068489276, 0.3759677031204938, 0.367967618582131, 0.3562141274449442, 0.3409063856703304, 0.3222770694191202, 0.300589321899439, 0.276029453368387, 0.2489556489584998, 0.2198307781323001, 0.189007744098907, 0.1568496585105438, 0.1237258553240216, 0.0900079246643524, 0.05606580563623959, 0.0222639756971989, -0.01104222743967856, -0.04351011734570545, -0.07481297204573968, -0.1046431194681109, -0.1326845439709606, -0.158529892304585, -0.1820833407428039, -0.2031487682490108, -0.2215607697352188, -0.23718568472133, -0.2499222783878022, -0.2597020756507648, -0.2664893522065585, -0.2702807897102097, -0.271104805344642, -0.269020568980866, -0.2641167239017489, -0.2565098296427363, -0.2461664691986244, -0.2334318509443001, -0.2185352049631993, -0.2016890083122531, -0.183121160056417, -0.1630722233343206, -0.1417926004200845, -0.1195396707012361, -0.09657492122667069, -0.0731610989457923, -0.04955941296931107, -0.0260268141445458, -0.002813377966465623, 0.01980231389058841, 0.04156349322445071, 0.06228431089767503, 0.08177148559093286, 0.09984999199421705, 0.1163644440263878, 0.1311802392691375, 0.1441844585572301, 0.1552865172067159, 0.1644185668833238, 0.1715356495934695, 0.1766156076996928, 0.1796587562009869, 0.1806525157574978, 0.1795443694021465, 0.1765460483336899, 0.171745449115216, 0.1652473357047022, 0.1571717520983067, 0.1476523155358822, 0.1368344108995743, 0.1248733074391872, 0.1119322192498233, 0.09818033100976004, 0.08379081036275189, 0.06893882800358016, 0.05379960600482305, 0.03855313055133823, 0.02339283528397695, 0.008481553504529806, -0.006026733850123303, -0.019987268614142, -0.03326581314067525, -0.04573979799289703, -0.0572993114637636, -0.06784792462253475, -0.07730334741902092, -0.08559791321314876, -0.09267889092136007, -0.09850862576639074, -0.1030016101736107, -0.106164427756163, -0.1080581950472024, -0.1087077745530493, -0.1081509714766913, -0.1064376886402523, -0.1036289744186825, -0.09979597701775561, -0.09501881905512868, -0.0893854068757596, -0.08299018935137888, -0.0759328810781533, -0.06831716489845648, -0.06024253143748921, -0.05181398020833086, -0.04316933968449398, -0.03441433275835372, -0.0256518465258018, -0.0169808618685762, -0.008495463173396752, -0.0002839366963369731, 0.007572035107996071, 0.01499807720583021, 0.02192771872991397, 0.0283028793126941, 0.03407424275822313, 0.03920151708834404, 0.04359727683349487, 0.04728862075454274, 0.05027085825707135, 0.05254282953379308, 0.05411182701557619, 0.05499316946360908, 0.05520969690174127, 0.05479119478305571, 0.05377375632420051, 0.05219909237303354, 0.05011379849803205, 0.04756858920091731, 0.04461750925721056, 0.04130244709440997, 0.03769528841981392, 0.03386777284739473, 0.02987889827463035, 0.02578672336006873, 0.02164767729433402, 0.01751591651948347, 0.01344273453290262, 0.009476030117783436, 0.005659838511713966, 0.0020339291670238, -0.001366527119689695, -0.004511219816904536, -0.007367729871474806, -0.009901861550492311, -0.01211859984281217, -0.01400985201938325, -0.01557252278049112, -0.01680831923508257, -0.01772349997493578, -0.01832857332032899, -0.0186379502689717, -0.01866955806169344, -0.01844442058462725, -0.01798621205697519, -0.01732079060517056, -0.01647571839833739, -0.01547010684523858, -0.01434816073608636, -0.01314022094787925, -0.01187548976028698, -0.01058231389629135, -0.009287782309770584, -0.008017356663036716, -0.006794538429417281, -0.005640575996588376, -0.00457421456190775, -0.003611491007959849, -0.002765575331476396, -0.002046659578307503, -0.001469507359562305, -0.001035241075342001, -0.0007395248899375738, -0.0005796106435392562, -0.0005500126651351602, -0.0006426905573889731, -0.0008472659506793363, -0.001151269400397902, -0.001540413306381637, -0.001998886491976192, -0.002509665894128177, -0.003054840686015736, -0.003615944080452104, -0.004173660523772592, -0.004706325358489177, -0.005196257331795073, -0.005626464945564855, -0.005981135743103662, -0.00624589949888101, -0.006408065706624307, -0.006456832497095437, -0.006383464528305579, -0.006181437823437308, -0.005846549980523641, -0.005376994638100455, -0.004773399547690802, -0.004038828072190172, -0.0031687430101222, -0.002182768802998294, -0.001091801816840697, 9.149438846438936e-05, 0.001352642604016473, 0.002675562999744606, 0.00404280653584551, 0.005435807459716977, 0.006835151391745014, 0.008220855347019008, 0.009572655927029695, 0.01087030184628287, 0.01209384693376114, 0.0132172900399709, 0.01422261939087458, 0.01509585219275619, 0.01582165264556101, 0.01638642434653658, 0.01677851122246396, 0.01698837287703214, 0.01700873224871682, 0.01683469382207408, 0.01646383099864786, 0.01589624160891694, 0.01513457093003053, 0.01418400196160332, 0.01304720586885398, 0.01173100396795205, 0.01025913219194152, 0.008646792466628393, 0.006911102932615302, 0.005070893441630774, 0.003146479657808526, 0.001159418561862167, -0.0008677516490630938, -0.002911784112122845, -0.004949005020445043, -0.006955601146148806, -0.008907909741745689, -0.01078259562316814, -0.01254628565590196, -0.01418464832758821, -0.01567797210834128, -0.01700816734617024, -0.01815898508324077, -0.01911621377265355, -0.01986785170614643, -0.02040425324742173, -0.02071824726765, -0.02080522649605138, -0.0206632068261885, -0.02029285595453149, -0.01969749106874409, -0.01887032333949126, -0.0178287502509513, -0.01659042861190782, -0.01516929631589021, -0.01358138463892434, -0.01184462908832521, -0.009978659003526938, -0.008004568381767708, -0.005944670589750726, -0.003822239782104063, -0.001661241977287212, 0.00051394115946856, 0.002678792709312009, 0.004806936569313566, 0.00686994428143983, 0.008846877191963216, 0.01071589621373403, 0.01245648985218492, 0.01404970136492851, 0.0154783370453054, 0.0167271534404842, 0.01778302155568584, 0.01863506635495082, 0.01927478014305973, 0.01969610870011399, 0.01989550933717429, 0.01987108565876816, 0.01960711886024268, 0.01912723538655214, 0.0184386889196778, 0.01755104492770126, 0.01647605386482175, 0.01522750078185147, 0.01382103319253205, 0.01227396925759296, 0.01060508854251793, 0.008834407774894512, 0.006982944171591036, 0.005072469023626815, 0.003125254316538516, 0.001164579408309537, -0.0007838476924446568, -0.00269768635790953, -0.004555544644619493, -0.006336907754929609, -0.008022363036967285, -0.009593810381396808, -0.01103465582470173, -0.01232998636664659, -0.01346672422603686, -0.01443375899173061, -0.01522205637281123, -0.01582474251054044, -0.01623033309104573, -0.01643313355526369, -0.01644378096447758, -0.01626506647292855, -0.01590193706130939, -0.01536141145705188, -0.01465247344404166, -0.01378594398858882, -0.01277433382195874, -0.01163167831464821, -0.01037335665220944, -0.009015897475290736, -0.007576773276364373, -0.006073886377628722, -0.004524754665110819, -0.002953538678451534, -0.001379201229685315, 0.0001795647216894178, 0.001704551063514076, 0.00317824110036806, 0.004584005472952154, 0.005906285342788432, 0.007130760917519917, 0.008244503570363175, 0.009236110002038622, 0.01009581710282004, 0.01081559639414924, 0.01137914051782371, 0.01178861228074839, 0.01204528432186032, 0.01214916835103653, 0.01210212581208079};
const Float iShY[] = {
    0, -0.1292114954114609, -0.256075757415927, -0.3784956151631083, -0.4944503350568962, -0.6020280149695963, -0.6994561922107971, -0.7851302003733032, -0.8576388445967151, -0.9157870051001227, -0.9586148245050755, -0.9854131849064285, -0.9957352352075171, -0.9894037872262597, -0.96555204513834, -0.9251668313971924, -0.869432660072939, -0.7993125405484989, -0.716002074440196, -0.6209095497644533, -0.5156327424361922, -0.4019327905243273, -0.2817055486460274, -0.1569508660336397, -0.0297402617127973, 0.09781650648627153, 0.2236054660000621, 0.3453502713837453, 0.4606261349475921, 0.5677740651061467, 0.6650527661674767, 0.7508891889401269, 0.823903153972962, 0.882928833247614, 0.9270327749574337, 0.9555282082978966, 0.9679854209068846, 0.9642380600024596, 0.9443852686170832, 0.9087896298477397, 0.858029164713678, 0.7919197890390489, 0.7129205184598609, 0.6224039682093042, 0.5219238822451825, 0.4131891806722102, 0.2980357035738437, 0.1783960965980639, 0.05626830844649156, -0.06631681146339945, -0.1873283156499338, -0.3047678577660267, -0.4167017873498224, -0.5212921488891429, -0.6161453785350176, -0.6998863155320247, -0.7714317006448205, -0.8296724003186529, -0.873723526499117, -0.9029372888372423, -0.9169120339424477, -0.91549735694613, -0.8987952291308505, -0.8671571443004218, -0.8211773451595382, -0.7616822484917969, -0.6897162436390172, -0.6061946975074877, -0.5125710331143969, -0.4110686688800241, -0.3034247710987056, -0.191464190773831, -0.0770694642528055, 0.03784987348054181, 0.1513881881459749, 0.261674383817201, 0.3669018115390187, 0.465356995000429, 0.555446739032698, 0.6357232123535722, 0.7048341018914758, 0.760662206749663, 0.8033052669145021, 0.8321690388227566, 0.8468896612917467, 0.8473375504471936, 0.8336175710522897, 0.8060655135800392, 0.7652409604699841, 0.7119166774811408, 0.6470647162404695, 0.5718394613662785, 0.4875578993299108, 0.395677425959239, 0.2975039029973983, 0.1951696910778097, 0.09054207398300801, -0.01462397176183999, -0.1185811086462839, -0.2196176980651272, -0.3160850381893273, -0.4064233703163854, -0.4891862660134684, -0.5630630343782994, -0.6268988205661091, -0.6797121029269971, -0.720709336182514, -0.7488183637321908, -0.7634615921604984, -0.7651680715550333, -0.754046167306046, -0.7304131846602031, -0.6947880752756983, -0.6478810400003654, -0.5905802191100985, -0.523935702171364, -0.4491411269800162, -0.3675131702391835, -0.2804692613792342, -0.1895038748539656, -0.09615815464505112, -0.002128064642871876, 0.09083211491133256, 0.1811917214000258, 0.2674804919806911, 0.3483115754765903, 0.4224029637178752, 0.488597031847151, 0.545877908481136, 0.5933864315281032, 0.6304324834156839, 0.656504539972957, 0.6712763096976987, 0.6746103840681926, 0.6657983339210897, 0.6457042882809135, 0.6150261975778881, 0.5744070502936928, 0.524645021549044, 0.4666786802302523, 0.4015701573507344, 0.330486558233452, 0.2546799237013073, 0.175466063559538, 0.09420259906363809, 0.01226655965844866, -0.06896811702716463, -0.1480109782785373, -0.2233625107237471, -0.2939191633336361, -0.3585715576500209, -0.4163223488219156, -0.4663008561689404, -0.5077754971486659, -0.5401638622284189, -0.5630403050563967, -0.5761409605138317, -0.5793661422061248, -0.572780110210184, -0.5566082389313994, -0.5311300075444779, -0.4963945608065448, -0.4538124954533618, -0.4042095027982753, -0.3485148848799123, -0.2877450125727687, -0.2229856394588894, -0.1553733596872143, -0.08607650671113022, -0.01627579419789019, 0.05285499945519906, 0.1201680056364502, 0.1845596099007786, 0.2449878892286096, 0.3000640446249813, 0.349115313449841, 0.3914984304786701, 0.4266084101336042, 0.4539695721199625, 0.4732413131912202, 0.4842216487317894, 0.4868485018522736, 0.4811987513676749, 0.4674850830909559, 0.4460507209056566, 0.4173621446821619, 0.381999930891777, 0.3404278253642017, 0.2935360211292384, 0.2424634680725181, 0.1881353917555302, 0.1315157535374714, 0.07359094564482749, 0.01535338728057109, -0.04221472320970214, -0.09815725127369947, -0.1515593249222962, -0.2015616628646026, -0.2473738213615038, -0.2882861677603631, -0.3235905832376889, -0.3523683177236021, -0.3746514781304462, -0.390180041173639, -0.3988105381461771, -0.4005164003214035, -0.3953864334449462, -0.3836214758879907, -0.3655293211765607, -0.3415180102088095, -0.3120876212312559, -0.2778207062966294, -0.2393715412283519, -0.1974543718560736, -0.152699151318721, -0.1061553499125537, -0.05867619120488202, -0.01107929589456277, 0.0358305736017983, 0.08127445306555175, 0.1245114783858299, 0.1648503690676139, 0.2016599281176045, 0.2343784103536616, 0.2625216302876605, 0.285689701402415, 0.3035723205948275, 0.3156927951885631, 0.3219525341487866, 0.3225887102857271, 0.3177001820966727, 0.307476150740337, 0.2921913546264507, 0.272199978735212, 0.2479283967513978, 0.2198668795214204, 0.1885604166186888, 0.1545988087953438, 0.1186061976957015, 0.08123020532635619, 0.04313109180305592, 0.005046013348670195, -0.03232998541828506, -0.06837711546110109, -0.1025103894252113, -0.1341885707835724, -0.1629222435295497, -0.1882808928028344, -0.2098989037340295, -0.227480403707688, -0.2408028919217243, -0.2497196193083036, -0.2541607013201733, -0.2541329655192111, -0.2493936077218674, -0.2404216930906825, -0.2275077170957802, -0.2109564105129734, -0.191127312363099, -0.1684276897187501, -0.1433048263771529, -0.1162378139935951, -0.08772898428756148, -0.05829512388421468, -0.02845861423261976, 0.001261362132080256, 0.03035741090041843, 0.058261930559709, 0.08446946524481989, 0.1086137843474659, 0.130328252943658, 0.1492954541676588, 0.1652514011739375, 0.1779887902323883, 0.1873592632129905, 0.1932746633924735, 0.1957072841258792, 0.1946891252662124, 0.1903101870880022, 0.1827158456854726, 0.172046733127502, 0.1584545466135108, 0.1424422007508331, 0.1243428864976271, 0.1045174640936887, 0.08334779470382073, 0.06122987699301322, 0.03856690201503864, 0.01576233867176881, -0.006786840720962338, -0.02869668947737269, -0.04960247061246364, -0.06916440340739852, -0.08707288822638222, -0.1028761160931553, -0.1164646916788045, -0.127674255825862, -0.1363704142726455, -0.1424650497636416, -0.1459167046934408, -0.1467301854647339, -0.1449554121823936, -0.1406855484265582, -0.1340544562409099, -0.1252335310128742, -0.1144279794968174, -0.1018726117387704, -0.08776655907006731, -0.07245383902139876, -0.05628512832114022, -0.03956466645294461, -0.02259831312293738, -0.005688317419405343, 0.01087172566996683, 0.02680165428157259, 0.04183918296942596, 0.05574394575097774, 0.06830108195328825, 0.07932431617030369, 0.08865849247854453, 0.09613651709281179, 0.1015872042430567, 0.1050866722255972, 0.106628767488658, 0.1062415106660555, 0.1039857048011442, 0.09995302908557599, 0.09426366191756601, 0.08706348343527689, 0.0785209131479313, 0.06882344281452329, 0.05817392826370327, 0.04678670637699588, 0.03488360495193706, 0.02268783445800119, 0.01046704601738935, -0.001560272165034802, -0.01318672187406148, -0.02421801108134798, -0.03447595793267484, -0.04380114031354235, -0.05205515613277014, -0.05912246648223947, -0.06491180111221481, -0.06935711310067427, -0.07241807708627966, -0.07408013287350891, -0.07428017016965463, -0.07309290499656965, -0.07062983007539642, -0.06697700711689622, -0.06223903239094868, -0.05653663885009984, -0.0500040869596879, -0.04278639370906394, -0.03503645066193947, -0.02691208248164788, -0.01857309714780742, -0.0101783780826435, -0.001883066655384124, 0.006153382488903103, 0.01376121675349466, 0.02083171663102371, 0.02725461487556679, 0.03293519323822836, 0.03779549395846774, 0.04177519914808908, 0.04483217216883102, 0.0469426608151302, 0.04810116769216225, 0.04831999854977075, 0.04762850442333334, 0.0460720381755064, 0.04371065036579317, 0.04056320147514345, 0.03678596177517503, 0.03247653156103247, 0.02773764047305458, 0.02267636696890906, 0.01740199854003413, 0.01202391668262556, 0.006649543251172933, 0.001382382872420094, -0.003679806357520793, -0.008446684584968852, -0.01283681833563024, -0.01677896277170907, -0.02018057869842182, -0.02300419438289468, -0.02523415348108214, -0.02685223711852038, -0.02785305270172234, -0.02824369578732477, -0.0280431861519786, -0.02728169448938801, -0.02599957938395585, -0.02424625704515353, -0.0220789286989429, -0.01956119249414504, -0.01676156827037698, -0.01374821949031762, -0.01060068823354806, -0.007404712644501177, -0.004231888477267297, -0.001150470399004466, 0.001775919693772389, 0.004489455457129635, 0.006939121339567379, 0.00908156468568638, 0.01088177235153647, 0.0123135628651866, 0.01335988864251886, 0.01401294628204903, 0.01427368085966075, 0.01412164637823022, 0.01361225007597743, 0.01277418493313557, 0.01164277010083822, 0.01025899230583827, 0.008668453360210599, 0.006920246228035578, 0.0050657829695381, 0.003157598344038191, 0.001248152906089098, -0.0006113409206880091, -0.002372047050203928, -0.003988566599384504, -0.005406169095344735, -0.006592178700293475, -0.007524205282636448, -0.008181542569964121, -0.008550653605418197, -0.00862536904888252, -0.008406926614968962, -0.007903853035137288, -0.007131692815801075, -0.006112590840144569, -0.004874738487591709, -0.003451695382772929, -0.001881601099482853, -0.0002045218430383534, 0.001530090059604219, 0.003269987852534888, 0.004968538734122515, 0.006579992241455741, 0.008060506921828491, 0.009369131719184428, 0.01046872308409488, 0.01132678014996793, 0.01191618193917762, 0.01221581244338012, 0.01221106153205527, 0.01189419194910989, 0.01126333123560479, 0.01030210976659108, 0.009052732046518313, 0.007537755355949699, 0.005785995676776658, 0.003831951900675842, 0.001715113852957567, -0.0005208323836528928, -0.002828886564684954, -0.005159686988093509, -0.007462515752498414, -0.009686337575711711, -0.01178085255788247, -0.01369754297831878, -0.01537334668912553, -0.01677111535263819, -0.01786097371714779, -0.01861335789750272, -0.01900518939358994, -0.01902040036657458, -0.01865032616932766, -0.01789395759246054, -0.01675804775277993, -0.01525707109330021, -0.01341303454532871, -0.01125514348530985, -0.008819327663846976, -0.006141206821007653, -0.003272233441963601, -0.0002778602049940807, 0.002785710511498461, 0.005860204336195678, 0.008886366532946716, 0.01180507800298343, 0.01455847572467559, 0.01709105761244659, 0.01935075201510101, 0.02128993262840939, 0.02286636045893148, 0.02404403563351998, 0.02479078859162535, 0.02505055523720454, 0.02484753726111765, 0.02417966953201595, 0.02305351441186026, 0.02148420320927729, 0.01949522329735267, 0.01711805426114388, 0.01439165895931132, 0.01136183780670968, 0.008080456876266441, 0.004604562546444316, 0.0009953973545557924, -0.002682666571994776, -0.006358181061584057, -0.009956135641150402, -0.01341041423476542, -0.01665699458205973, -0.01963515531970875};
const Float rShZ[] = {
    1, 0.9909053923078116, 0.9652637904600097, 0.9235264025840351, 0.8664070035151712, 0.7948699259650923, 0.7101141265330139, 0.6135535802050038, 0.5067943104217086, 0.3916084107925215, 0.269905458483958, 0.1437017576582019, 0.01508788359354928, -0.1138049761426015, -0.2405822242415946, -0.3629419555000865, -0.4789279722698351, -0.5866375598046702, -0.6843085650751415, -0.7703473562825227, -0.8433540439461378, -0.9021445856506642, -0.9457694436259358, -0.9735285158717859, -0.9849821168626923, -0.9799578422595412, -0.9585532127726879, -0.9206546336926464, -0.8666244683741575, -0.7983288783449659, -0.716953117015202, -0.6238915524917779, -0.5207243809002242, -0.4091915099115547, -0.2911640245712225, -0.168613681290205, -0.04358090326455677, 0.08185822868151091, 0.2056254820278691, 0.3256739607502954, 0.4399981294782062, 0.5458994338947658, 0.6421548626453049, 0.7272211702036838, 0.7997459971947826, 0.85858861523021, 0.9028372262314843, 0.9318225705516601, 0.9451276539365514, 0.9425934614106677, 0.9243205857770765, 0.8906667588159822, 0.8422403336818484, 0.7798898266592406, 0.7039787884246269, 0.61646305769696, 0.519187045002367, 0.4138196278794133, 0.3021516950707344, 0.1860665902977194, 0.06750928279044056, -0.05154525574119512, -0.1691239792907537, -0.2832875106139776, -0.3921612138368577, -0.4939651229761172, -0.587042270995904, -0.6694823969620917, -0.7395755381723244, -0.796805005237615, -0.8403176642076102, -0.8694908523285215, -0.8839407025575614, -0.8835266461508675, -0.8683520526853353, -0.8387610239991944, -0.795331415010443, -0.7388642094809177, -0.6703694318413262, -0.5910488265089969, -0.5022297272868745, -0.405012290050373, -0.3018504443620207, -0.1945024928165446, -0.08478027434720453, 0.02548044699457643, 0.1344497171841247, 0.2403327950886522, 0.3413987756735733, 0.436008001138613, 0.5226378484386617, 0.5999065081025001, 0.6665944009457436, 0.7216629157589317, 0.7634050615213042, 0.7916981128068531, 0.8064820755084666, 0.8076393934648517, 0.7952765833428386, 0.769720432523044, 0.7315107611627297, 0.6813898907129463, 0.6202890083491011, 0.5493116609086878, 0.4697146524794602, 0.3828866562512894, 0.2903248831902291, 0.1935249068838533, 0.0943236148637953, -0.005402077957055487, -0.1039930093211654, -0.1998260227977608, -0.2913397422503501, -0.3770591047371465, -0.4556182888264104, -0.5257817026300842, -0.5864627275738065, -0.6367399495843249, -0.6758706484999545, -0.7033013585737485, -0.7185671792432355, -0.7206607431937723, -0.710650573116128, -0.6888442038327593, -0.6557389725067633, -0.6120118075011476, -0.5585063004328009, -0.4962172906292395, -0.426273225239424, -0.3499165882483939, -0.2684827172613402, -0.1833773478694724, -0.09605324145522402, -0.007986263260733281, 0.07918243068322457, 0.1638843752705857, 0.2447593785307698, 0.3205110744761741, 0.3899433046692961, 0.4519782002400718, 0.505672176020721, 0.5502296162414877, 0.5850140678362749, 0.6095567961624555, 0.6235625983265186, 0.6269128107959022, 0.619665490027836, 0.6016548642279381, 0.5732896203776288, 0.5356756884698679, 0.489565532049401, 0.4358411311472696, 0.3754979936786421, 0.3096275816417243, 0.239398442723435, 0.1660363533196353, 0.09080378992323225, 0.01497905223570828, -0.06016463681970152, -0.1333797317470329, -0.2034125682701987, -0.2687294144493267, -0.3285481860691869, -0.3819518869807256, -0.4281425069729634, -0.4664522339070706, -0.4963524433504596, -0.5174603586072709, -0.5295433099594408, -0.532520558356576, -0.5264626852510204, -0.5115885862990072, -0.488260141782424, -0.4569746704031906, -0.4178932735367027, -0.3722595023432862, -0.3210585061784104, -0.265232820276373, -0.2057868755458442, -0.1437697631732237, -0.08025754522442143, -0.01633539057471859, 0.046920185849789, 0.1084587077515013, 0.1672724822631907, 0.2224124536410542, 0.2730029305871871, 0.3179909116773948, 0.3565953566087695, 0.3885147125080189, 0.413324881105598, 0.4307263044487036, 0.4405468336544414, 0.4427425247500993, 0.4373963814611929, 0.4247150949789694, 0.4050238597676317, 0.3787593720005716, 0.3464611429086273, 0.3087612828696678, 0.2663166162306931, 0.219780033551352, 0.1703440790123612, 0.1188905189132973, 0.0663192692308299, 0.01353340711086838, -0.03857561567839648, -0.08914233160010079, -0.1373409727936253, -0.1823983402849041, -0.2236056396887781, -0.2603291141098585, -0.2920193250948586, -0.3182189545202295, -0.3381117145492951, -0.3517939292509942, -0.3592296726715029, -0.3604087847823619, -0.3554254028658956, -0.344474566943767, -0.3278472691286743, -0.3059240502150628, -0.2791672668188249, -0.2481121703190559, -0.213356954545037, -0.1755519424141613, -0.1353880924171469, -0.09354831427537955, -0.05088045006006382, -0.008188403120087799, 0.03380533483825209, 0.07440377569041878, 0.112946398258656, 0.1488193240910796, 0.1814645611154687, 0.2103881871118154, 0.2351673629651825, 0.2554560849549578, 0.2709896056398384, 0.2815874739329076, 0.2870660288628287, 0.2871622565218552, 0.2823340856536493, 0.2727647417989447, 0.2587131906249059, 0.2405083664584051, 0.2185423850993267, 0.1932628664799186, 0.1651645037987447, 0.1347800246831891, 0.1026706966430395, 0.06941653351580709, 0.03560636174474743, 0.001827905171911956, -0.03124426775463644, -0.06304402371107756, -0.09306398172011628, -0.1208306412851515, -0.145918391457861, -0.1679555715508333, -0.1866295363758698, -0.2016906656840152, -0.2129552745691247, -0.2203073989956682, -0.2236994480906942, -0.2231517321691823, -0.2187508924153451, -0.2104777236334274, -0.1986190753968218, -0.1836010697287454, -0.165752285866502, -0.1454418748066625, -0.1230725383542642, -0.09907311054963804, -0.07389086693973335, -0.04798368880897039, -0.02181220927853774, 0.004167933841770255, 0.02951361735311396, 0.05380140849904894, 0.07659587432142548, 0.09740764117115358, 0.1160231941957806, 0.1321747644104553, 0.145642723297715, 0.1562581544596216, 0.1639045377127038, 0.1685185380663419, 0.1700899058608486, 0.1686605078093076, 0.1643225216158797, 0.1572158390710483, 0.1475247338993451, 0.1354738610174358, 0.1211828058176793, 0.1051295071946116, 0.08765172915973278, 0.06908752689526529, 0.049785270818508, 0.03009753421802411, 0.01037505249518476, -0.009039150772724158, -0.02781538032039452, -0.04564254342104937, -0.06223300398064303, -0.07732694658496299, -0.09069618943429146, -0.1020385039089121, -0.1112149055719974, -0.1181915635915529, -0.1229054302586474, -0.1253340667888303, -0.1254950018308012, -0.1234444412191034, -0.1192753650100822, -0.1131150565483084, -0.1051221161785381, -0.09548301915680725, -0.08440828324641146, -0.072128316346234, -0.05887317444109465, -0.04490448042198855, -0.03054729568238579, -0.01606448533063227, -0.001713854928086525, 0.01225613001062542, 0.02561035763141453, 0.03813075755438913, 0.04961961046931666, 0.05990243422256791, 0.06883040875469576, 0.07628231030532578, 0.08216593365070744, 0.08641898966033394, 0.08887105188706827, 0.08965751700703237, 0.08882280617293446, 0.08643073920585818, 0.08257065565352531, 0.0773551492709324, 0.07091746738795066, 0.06340862667885425, 0.05499430013153169, 0.04585153240232059, 0.03616534222095679, 0.0261252710750528, 0.01592193706203196, 0.005756566923157176, -0.00416639083775322, -0.01367798574881198, -0.02262077920662858, -0.03085242944382167, -0.03824775006306567, -0.04470041374350564, -0.0501242819090367, -0.05445434766263776, -0.05764728584906705, -0.0596816106177695, -0.06055744722879147, -0.06029593098992195, -0.05891096444005019, -0.05643663567719096, -0.05302245822040044, -0.04876470074703465, -0.04377174625939083, -0.03816175337041186, -0.03206021443775436, -0.02559745528660434, -0.01890612107522585, -0.01211869198551687, -0.005365070890849473, 0.001229717000254786, 0.007547675261918345, 0.01347980413717826, 0.01887744329360085, 0.02369094692968705, 0.0278535051155215, 0.03131212373265254, 0.03402902689834856, 0.03598185256984435, 0.03716356135851154, 0.03758206807407902, 0.037259609956664, 0.03623186967146597, 0.03454687488453278, 0.03226369956050625, 0.02945099498304928, 0.02616473435147456, 0.02251174644351727, 0.01859359981782471, 0.0145000571776622, 0.01032084406269125, 0.006143905273230965, 0.002053748339719942, -0.001870098182736241, -0.005554486481509179, -0.008933889216559316, -0.01195145842610353, -0.01455988837475038, -0.01672207085211092, -0.01839709679956895, -0.01955480072545824, -0.02022162559407757, -0.02040662063494272, -0.02012870811367224, -0.01941596253062464, -0.01830472979081881, -0.01683860700427138, -0.01506730558771189, -0.0130454219348254, -0.01083114108650605, -0.008484899554637569, -0.006068033727911746, -0.003641500150788778, -0.001274591056311301, 0.0009769926193654629, 0.003063013286528747, 0.004938812150039658, 0.006566128152549319, 0.007913765267886433, 0.008958098391919047, 0.009683411203845669, 0.01008206255258066, 0.01015448111158035, 0.009908991192716933, 0.009361475668341375, 0.008534884874717786, 0.007445926270237812, 0.006143328442534334, 0.004674831427165644, 0.003084733077679933, 0.001419591735865177, -0.0002728684097802925, -0.001944860980227558, -0.003549628116360557, -0.005042496809570114, -0.006381880301868748, -0.007530205227948004, -0.008454746712351965, -0.009128355467563194, -0.009520735640506938, -0.009610150664524624, -0.00940675872923177, -0.008912661786945912, -0.008137035348572026, -0.007095906838542512, -0.005811791178754432, -0.004313191756502464, -0.002633977335837745, -0.0008126476761698427, 0.001108497405379428, 0.003084269078731194, 0.005067562775871413, 0.007009998001884457, 0.008851576825604734, 0.01054975704370273, 0.01206123971075896, 0.01334634182218813, 0.01436986657134984, 0.01510188399811964, 0.01551840719877291, 0.01560195121156967, 0.01534196384064136, 0.01473511999889282, 0.0137854736029681, 0.01250446360262534, 0.01091077333469931, 0.009014177628605831, 0.006863270076393271, 0.004504752495181326, 0.001983263826878375, -0.0006525243348943289, -0.003350843018069575, -0.006057826083480508, -0.00871855698155484, -0.01127814285491161, -0.01368279629650705, -0.01588090490400074, -0.01782406892599915, -0.01946808777267401, -0.02076058987786918, -0.02165858373214535, -0.02215590605542558, -0.02223593190116548, -0.02188990308010527, -0.02111716653807134, -0.01992526354866243, -0.01832986771502027, -0.01635457235767356, -0.01403053043467508, -0.01139595265862053, -0.008495471906119028, -0.00537938432345599, -0.002102113396859601, 0.001278011404083152, 0.004685749695129356, 0.00805775451492465, 0.01133071548394139, 0.01444252190161439, 0.0173334096243558, 0.01994707169953802, 0.02223171333284784, 0.02414103267223284, 0.02563511008700669, 0.02668119008811821, 0.02725434175434219, 0.02733798547531899, 0.0268912375781862, 0.02593983192715625, 0.02450743159749482, 0.02261583273414464, 0.02029528247893199};
const Float iShZ[] = {
    0, -0.1292114954114609, -0.256075757415927, -0.3784956151631083, -0.4944503350568962, -0.6020280149695963, -0.6994561922107971, -0.7851302003733032, -0.8576388445967151, -0.9157870051001227, -0.9586148245050755, -0.9854131849064285, -0.9957352352075171, -0.9894037872262597, -0.96555204513834, -0.9251668313971924, -0.869432660072939, -0.7993125405484989, -0.716002074440196, -0.6209095497644533, -0.5156327424361922, -0.4019327905243273, -0.2817055486460274, -0.1569508660336397, -0.0297402617127973, 0.09781650648627153, 0.2236054660000621, 0.3453502713837453, 0.4606261349475921, 0.5677740651061467, 0.6650527661674767, 0.7508891889401269, 0.823903153972962, 0.882928833247614, 0.9270327749574337, 0.9555282082978966, 0.9679854209068846, 0.9642380600024596, 0.9443852686170832, 0.9087896298477397, 0.858029164713678, 0.7919197890390489, 0.7129205184598609, 0.6224039682093042, 0.5219238822451825, 0.4131891806722102, 0.2980357035738437, 0.1783960965980639, 0.05626830844649156, -0.06631681146339945, -0.1873283156499338, -0.3047678577660267, -0.4167017873498224, -0.5212921488891429, -0.6161453785350176, -0.6998863155320247, -0.7714317006448205, -0.8296724003186529, -0.873723526499117, -0.9029372888372423, -0.9169120339424477, -0.91549735694613, -0.8987952291308505, -0.8671571443004218, -0.8211773451595382, -0.7616822484917969, -0.6897162436390172, -0.6061946975074877, -0.5125710331143969, -0.4110686688800241, -0.3034247710987056, -0.191464190773831, -0.0770694642528055, 0.03784987348054181, 0.1513881881459749, 0.261674383817201, 0.3669018115390187, 0.465356995000429, 0.555446739032698, 0.6357232123535722, 0.7048341018914758, 0.760662206749663, 0.8033052669145021, 0.8321690388227566, 0.8468896612917467, 0.8473375504471936, 0.8336175710522897, 0.8060655135800392, 0.7652409604699841, 0.7119166774811408, 0.6470647162404695, 0.5718394613662785, 0.4875578993299108, 0.395677425959239, 0.2975039029973983, 0.1951696910778097, 0.09054207398300801, -0.01462397176183999, -0.1185811086462839, -0.2196176980651272, -0.3160850381893273, -0.4064233703163854, -0.4891862660134684, -0.5630630343782994, -0.6268988205661091, -0.6797121029269971, -0.720709336182514, -0.7488183637321908, -0.7634615921604984, -0.7651680715550333, -0.754046167306046, -0.7304131846602031, -0.6947880752756983, -0.6478810400003654, -0.5905802191100985, -0.523935702171364, -0.4491411269800162, -0.3675131702391835, -0.2804692613792342, -0.1895038748539656, -0.09615815464505112, -0.002128064642871876, 0.09083211491133256, 0.1811917214000258, 0.2674804919806911, 0.3483115754765903, 0.4224029637178752, 0.488597031847151, 0.545877908481136, 0.5933864315281032, 0.6304324834156839, 0.656504539972957, 0.6712763096976987, 0.6746103840681926, 0.6657983339210897, 0.6457042882809135, 0.6150261975778881, 0.5744070502936928, 0.524645021549044, 0.4666786802302523, 0.4015701573507344, 0.330486558233452, 0.2546799237013073, 0.175466063559538, 0.09420259906363809, 0.01226655965844866, -0.06896811702716463, -0.1480109782785373, -0.2233625107237471, -0.2939191633336361, -0.3585715576500209, -0.4163223488219156, -0.4663008561689404, -0.5077754971486659, -0.5401638622284189, -0.5630403050563967, -0.5761409605138317, -0.5793661422061248, -0.572780110210184, -0.5566082389313994, -0.5311300075444779, -0.4963945608065448, -0.4538124954533618, -0.4042095027982753, -0.3485148848799123, -0.2877450125727687, -0.2229856394588894, -0.1553733596872143, -0.08607650671113022, -0.01627579419789019, 0.05285499945519906, 0.1201680056364502, 0.1845596099007786, 0.2449878892286096, 0.3000640446249813, 0.349115313449841, 0.3914984304786701, 0.4266084101336042, 0.4539695721199625, 0.4732413131912202, 0.4842216487317894, 0.4868485018522736, 0.4811987513676749, 0.4674850830909559, 0.4460507209056566, 0.4173621446821619, 0.381999930891777, 0.3404278253642017, 0.2935360211292384, 0.2424634680725181, 0.1881353917555302, 0.1315157535374714, 0.07359094564482749, 0.01535338728057109, -0.04221472320970214, -0.09815725127369947, -0.1515593249222962, -0.2015616628646026, -0.2473738213615038, -0.2882861677603631, -0.3235905832376889, -0.3523683177236021, -0.3746514781304462, -0.390180041173639, -0.3988105381461771, -0.4005164003214035, -0.3953864334449462, -0.3836214758879907, -0.3655293211765607, -0.3415180102088095, -0.3120876212312559, -0.2778207062966294, -0.2393715412283519, -0.1974543718560736, -0.152699151318721, -0.1061553499125537, -0.05867619120488202, -0.01107929589456277, 0.0358305736017983, 0.08127445306555175, 0.1245114783858299, 0.1648503690676139, 0.2016599281176045, 0.2343784103536616, 0.2625216302876605, 0.285689701402415, 0.3035723205948275, 0.3156927951885631, 0.3219525341487866, 0.3225887102857271, 0.3177001820966727, 0.307476150740337, 0.2921913546264507, 0.272199978735212, 0.2479283967513978, 0.2198668795214204, 0.1885604166186888, 0.1545988087953438, 0.1186061976957015, 0.08123020532635619, 0.04313109180305592, 0.005046013348670195, -0.03232998541828506, -0.06837711546110109, -0.1025103894252113, -0.1341885707835724, -0.1629222435295497, -0.1882808928028344, -0.2098989037340295, -0.227480403707688, -0.2408028919217243, -0.2497196193083036, -0.2541607013201733, -0.2541329655192111, -0.2493936077218674, -0.2404216930906825, -0.2275077170957802, -0.2109564105129734, -0.191127312363099, -0.1684276897187501, -0.1433048263771529, -0.1162378139935951, -0.08772898428756148, -0.05829512388421468, -0.02845861423261976, 0.001261362132080256, 0.03035741090041843, 0.058261930559709, 0.08446946524481989, 0.1086137843474659, 0.130328252943658, 0.1492954541676588, 0.1652514011739375, 0.1779887902323883, 0.1873592632129905, 0.1932746633924735, 0.1957072841258792, 0.1946891252662124, 0.1903101870880022, 0.1827158456854726, 0.172046733127502, 0.1584545466135108, 0.1424422007508331, 0.1243428864976271, 0.1045174640936887, 0.08334779470382073, 0.06122987699301322, 0.03856690201503864, 0.01576233867176881, -0.006786840720962338, -0.02869668947737269, -0.04960247061246364, -0.06916440340739852, -0.08707288822638222, -0.1028761160931553, -0.1164646916788045, -0.127674255825862, -0.1363704142726455, -0.1424650497636416, -0.1459167046934408, -0.1467301854647339, -0.1449554121823936, -0.1406855484265582, -0.1340544562409099, -0.1252335310128742, -0.1144279794968174, -0.1018726117387704, -0.08776655907006731, -0.07245383902139876, -0.05628512832114022, -0.03956466645294461, -0.02259831312293738, -0.005688317419405343, 0.01087172566996683, 0.02680165428157259, 0.04183918296942596, 0.05574394575097774, 0.06830108195328825, 0.07932431617030369, 0.08865849247854453, 0.09613651709281179, 0.1015872042430567, 0.1050866722255972, 0.106628767488658, 0.1062415106660555, 0.1039857048011442, 0.09995302908557599, 0.09426366191756601, 0.08706348343527689, 0.0785209131479313, 0.06882344281452329, 0.05817392826370327, 0.04678670637699588, 0.03488360495193706, 0.02268783445800119, 0.01046704601738935, -0.001560272165034802, -0.01318672187406148, -0.02421801108134798, -0.03447595793267484, -0.04380114031354235, -0.05205515613277014, -0.05912246648223947, -0.06491180111221481, -0.06935711310067427, -0.07241807708627966, -0.07408013287350891, -0.07428017016965463, -0.07309290499656965, -0.07062983007539642, -0.06697700711689622, -0.06223903239094868, -0.05653663885009984, -0.0500040869596879, -0.04278639370906394, -0.03503645066193947, -0.02691208248164788, -0.01857309714780742, -0.0101783780826435, -0.001883066655384124, 0.006153382488903103, 0.01376121675349466, 0.02083171663102371, 0.02725461487556679, 0.03293519323822836, 0.03779549395846774, 0.04177519914808908, 0.04483217216883102, 0.0469426608151302, 0.04810116769216225, 0.04831999854977075, 0.04762850442333334, 0.0460720381755064, 0.04371065036579317, 0.04056320147514345, 0.03678596177517503, 0.03247653156103247, 0.02773764047305458, 0.02267636696890906, 0.01740199854003413, 0.01202391668262556, 0.006649543251172933, 0.001382382872420094, -0.003679806357520793, -0.008446684584968852, -0.01283681833563024, -0.01677896277170907, -0.02018057869842182, -0.02300419438289468, -0.02523415348108214, -0.02685223711852038, -0.02785305270172234, -0.02824369578732477, -0.0280431861519786, -0.02728169448938801, -0.02599957938395585, -0.02424625704515353, -0.0220789286989429, -0.01956119249414504, -0.01676156827037698, -0.01374821949031762, -0.01060068823354806, -0.007404712644501177, -0.004231888477267297, -0.001150470399004466, 0.001775919693772389, 0.004489455457129635, 0.006939121339567379, 0.00908156468568638, 0.01088177235153647, 0.0123135628651866, 0.01335988864251886, 0.01401294628204903, 0.01427368085966075, 0.01412164637823022, 0.01361225007597743, 0.01277418493313557, 0.01164277010083822, 0.01025899230583827, 0.008668453360210599, 0.006920246228035578, 0.0050657829695381, 0.003157598344038191, 0.001248152906089098, -0.0006113409206880091, -0.002372047050203928, -0.003988566599384504, -0.005406169095344735, -0.006592178700293475, -0.007524205282636448, -0.008181542569964121, -0.008550653605418197, -0.00862536904888252, -0.008406926614968962, -0.007903853035137288, -0.007131692815801075, -0.006112590840144569, -0.004874738487591709, -0.003451695382772929, -0.001881601099482853, -0.0002045218430383534, 0.001530090059604219, 0.003269987852534888, 0.004968538734122515, 0.006579992241455741, 0.008060506921828491, 0.009369131719184428, 0.01046872308409488, 0.01132678014996793, 0.01191618193917762, 0.01221581244338012, 0.01221106153205527, 0.01189419194910989, 0.01126333123560479, 0.01030210976659108, 0.009052732046518313, 0.007537755355949699, 0.005785995676776658, 0.003831951900675842, 0.001715113852957567, -0.0005208323836528928, -0.002828886564684954, -0.005159686988093509, -0.007462515752498414, -0.009686337575711711, -0.01178085255788247, -0.01369754297831878, -0.01537334668912553, -0.01677111535263819, -0.01786097371714779, -0.01861335789750272, -0.01900518939358994, -0.01902040036657458, -0.01865032616932766, -0.01789395759246054, -0.01675804775277993, -0.01525707109330021, -0.01341303454532871, -0.01125514348530985, -0.008819327663846976, -0.006141206821007653, -0.003272233441963601, -0.0002778602049940807, 0.002785710511498461, 0.005860204336195678, 0.008886366532946716, 0.01180507800298343, 0.01455847572467559, 0.01709105761244659, 0.01935075201510101, 0.02128993262840939, 0.02286636045893148, 0.02404403563351998, 0.02479078859162535, 0.02505055523720454, 0.02484753726111765, 0.02417966953201595, 0.02305351441186026, 0.02148420320927729, 0.01949522329735267, 0.01711805426114388, 0.01439165895931132, 0.01136183780670968, 0.008080456876266441, 0.004604562546444316, 0.0009953973545557924, -0.002682666571994776, -0.006358181061584057, -0.009956135641150402, -0.01341041423476542, -0.01665699458205973, -0.01963515531970875};

MTS_NAMESPACE_END
