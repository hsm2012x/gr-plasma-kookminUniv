/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "advanced_doppler_processing_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <chrono>

namespace gr {
namespace plasma {

advanced_doppler_processing::sptr advanced_doppler_processing::make(size_t num_pulse_cpi, size_t nfft)
{
    return gnuradio::make_block_sptr<advanced_doppler_processing_impl>(num_pulse_cpi, nfft);
}


/*
 * The private constructor
 */
advanced_doppler_processing_impl::advanced_doppler_processing_impl(size_t num_pulse_cpi, size_t nfft)
    : gr::block("advanced_doppler_processing",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi),
      d_fftsize(nfft)
{
    d_in_port = PMT_IN;
    d_out_port = PMT_OUT;
    d_meta = pmt::make_dict();
    d_data = pmt::make_c32vector(0, 0);
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

/*
 * Our virtual destructor.
 */
advanced_doppler_processing_impl::~advanced_doppler_processing_impl() {}

void advanced_doppler_processing_impl::handle_msg(pmt::pmt_t msg)
{
    if (this->nmsgs(d_in_port) > d_queue_depth) {
        return;
    }

    // Read the input PDU
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        samples = pmt::cdr(msg);

        // Update block parameters with new metadata
        if (pmt::dict_has_key(meta, d_n_pulse_cpi_key)) {
            d_num_pulse_cpi =
                pmt::to_long(pmt::dict_ref(meta, d_n_pulse_cpi_key, pmt::PMT_NIL));
        }
        d_meta = pmt::dict_update(d_meta, meta);


    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }

    // Get pointers to the input and output arrays
    size_t n = pmt::length(samples);
    size_t io(0);
    if (pmt::length(d_data) != n * d_fftsize / d_num_pulse_cpi)
        d_data = pmt::make_c32vector(n * d_fftsize / d_num_pulse_cpi, 0);
    const gr_complex* in = pmt::c32vector_elements(samples, io);
    gr_complex* out = pmt::c32vector_writable_elements(d_data, io);
    int nrow = n / d_num_pulse_cpi;
    int ncol = d_num_pulse_cpi;

    // Take an FFT across each row of the matrix to form a range-doppler map
    af::array rdm(af::dim4(nrow, ncol), reinterpret_cast<const af::cfloat*>(in));
    // The FFT function transforms each column of the input matrix by default,
    af::array dc_offset = af::mean(rdm, 0);  // Calculate DC means
    rdm = rdm - af::tile(dc_offset, 1, ncol);  // remove DC offset by sumin
    // so we need to transpose it to do the FFT across rows.
    rdm = rdm.T();
    rdm = af::fftNorm(rdm, 1.0, d_fftsize);
    rdm = ::plasma::fftshift(rdm, 0);
    rdm = rdm.T();
    rdm.host(out);
    // Send the data as a message
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
    // Reset the metadata output
    d_meta = pmt::make_dict();
}

void advanced_doppler_processing_impl::set_metadata_keys(const std::string& n_pulse_cpi_key,
                                                const std::string& doppler_fft_size_key)
{
    d_n_pulse_cpi_key = pmt::intern(n_pulse_cpi_key);
    d_doppler_fft_size_key = pmt::intern(doppler_fft_size_key);

    d_meta = pmt::dict_add(d_meta, d_doppler_fft_size_key, pmt::from_long(d_fftsize));
}

void advanced_doppler_processing_impl::set_msg_queue_depth(size_t depth) { d_queue_depth = depth; }

void advanced_doppler_processing_impl::set_backend(Device::Backend backend)
{
    switch (backend) {
    case Device::CPU:
        d_backend = AF_BACKEND_CPU;
        break;
    case Device::CUDA:
        d_backend = AF_BACKEND_CUDA;
        break;
    case Device::OPENCL:
        d_backend = AF_BACKEND_OPENCL;
        break;
    default:
        d_backend = AF_BACKEND_DEFAULT;
        break;
    }
    af::setBackend(d_backend);
}
} /* namespace plasma */
} /* namespace gr */
