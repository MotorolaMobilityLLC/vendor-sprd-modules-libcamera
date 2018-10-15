/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _CMR_PROP_H_
#define _CMR_PROP_H_

/*
 * 0:Default value,AE run normal
 * 1:Don't supply the AE statistics,will not call AE API
 */
#define PROP_ISP_AE_BYPASS         "persist.vendor.camera.bypass.ae"

/*
 * 0:Default value,AF run normal
 * 1:Don't supply the AF statistics,will not call AF API
 */
#define PROP_ISP_AF_BYPASS         "persist.vendor.camera.bypass.af"

/*
 * 0:Default value,AWB run normal
 * 1:Don't supply the AWB statistics,will not call AWB API
 */
#define PROP_ISP_AWB_BYPASS        "persist.vendor.camera.bypass.awb"

/*
 * 0:Default value,LSC run normal
 * 1:Don't supply the LSC statistics,will not call LSC API
 */
#define PROP_ISP_LSC_BYPASS        "persist.vendor.camera.bypass.lsc"

/*
 * 0:Default value,PDAF run normal
 * 1:Don't supply the PDAF statistics,will not call PDAF API
 */
#define PROP_ISP_PDAF_BYPASS       "persist.vendor.camera.bypass.pdaf"

/*
 * 0:Default value,AFL run normal
 * 1:Don't supply the AFL statistics,will not call AFL API
 */
#define PROP_ISP_AFL_BYPASS        "persist.vendor.camera.bypass.afl"

/*
 * 0:Default value,fail to open debug file
 * 1:Open the debug file like aem_stats and embed_line_stats
 */
#define PROP_ISP_FILE_DEBUG        "persist.vendor.camera.ispfp.debug"

/*
 * 0:Default value,not read otp
 * 1:Open the refocus.otp read otp from file
 */
#define PROP_ISP_REFOCUS_OTP        "persist.vendor.camera.refocus.otp"

/*
 * 0:Default value,not save isp param
 * 1:save isp param
 */
#define PROP_ISP_SAVE_PARAM        "persist.vendor.save.isp.param"

#endif
