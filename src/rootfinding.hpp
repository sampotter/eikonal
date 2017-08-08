#ifndef __ROOTFINDING_HPP__
#define __ROOTFINDING_HPP__

constexpr double default_tol = 1e-7;

struct rootfinder {
  rootfinder(double (* f)(double));
  virtual double find_root(double x0, double x1, double tol = default_tol)
    const = 0;
protected:
  double eval_f(double x) const;
private:
  double (* _f)(double);
};

struct secant: public rootfinder {
  using rootfinder::rootfinder;
  virtual double find_root(double x0, double x1, double tol = default_tol)
    const override final;
};

#endif // __ROOTFINDING_HPP__

// Local Variables:
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
