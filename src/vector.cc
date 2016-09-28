/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "vector.h"

#include <assert.h>

#include <iomanip>
#include <iostream>

#include "matrix.h"
#include "utils.h"

Vector::Vector(int64_t m) {
  m_ = m;
  data_ = new real[m];
}

// make a vector with only non-zero item at indices at lbs
Vector::Vector(int64_t m, const std::vector<int32_t>& lbs) 
  : m_(m), data_(new real[m])
{
  zero();
  for( auto l : lbs) {
    assert( l < m_);
    data_[l] = 1.0;
  }
}

Vector::~Vector() {
  delete[] data_;
}

void Vector::zero() {
  for (int64_t i = 0; i < m_; i++) {
    data_[i] = 0.0;
  }
}

void Vector::ones() {
  for (int64_t i = 0; i < m_; i++) {
    data_[i] = 1.0;
  }
}

void Vector::mul(real a) {
  for (int64_t i = 0; i < m_; i++) {
    data_[i] *= a;
  }
}

void Vector::addRow(const Matrix& A, int64_t i) {
  assert(i >= 0);
  assert(i < A.m_);
  assert(m_ == A.n_);
  for (int64_t j = 0; j < A.n_; j++) {
    data_[j] += A.data_[i * A.n_ + j];
  }
}


void Vector::addRow(const Matrix& A, int64_t i, real a) {
  assert(i >= 0);
  assert(i < A.m_);
  assert(m_ == A.n_);
  for (int64_t j = 0; j < A.n_; j++) {
    data_[j] += a * A.data_[i * A.n_ + j];
  }
}


void Vector::print() const{
  for( int32_t i = 0; i < m_; ++i) {
    std::cout << data_[i] << ' ';
  }
  std::cout << '\n';
}

void Vector::mul(const Matrix& A, const Vector& vec) {
  assert(A.m_ == m_);
  assert(A.n_ == vec.m_);
  for (int64_t i = 0; i < m_; i++) {
    data_[i] = 0.0;
    for (int64_t j = 0; j < A.n_; j++) {
      data_[i] += A.data_[i * A.n_ + j] * vec.data_[j];
    }
  }
}

int64_t Vector::argmax() {
  real max = data_[0];
  int64_t argmax = 0;
  for (int64_t i = 1; i < m_; i++) {
    if (data_[i] > max) {
      max = data_[i];
      argmax = i;
    }
  }
  return argmax;
}

real& Vector::operator[](int64_t i) {
  return data_[i];
}

const real& Vector::operator[](int64_t i) const {
  return data_[i];
}

std::ostream& operator<<(std::ostream& os, const Vector& v)
{
  os << std::setprecision(5);
  for (int64_t j = 0; j < v.m_; j++) {
    os << v.data_[j] << ' ';
  }
  return os;
}
