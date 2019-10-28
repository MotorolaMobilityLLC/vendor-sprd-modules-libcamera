/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
    DESCRIPTION: configure multic camera info
 *
 */

    // 1 multiCameraMode
    MODE_SOFY_OPTICAL_ZOOM,

    // 2 configure virtual camera real id
    2,

    // 3 How many camera to open,support max value is 4
    2,

    // 4 hal_buffer_info     buffer_info
    /*ion buffer info configure. set hal layer allocate memory number,width and
       heigh*/
    {
        /*********************************************************************/
        //
        // Variable name: number
        // Data range:
        // How many number buffer to request

        // Variable name: width
        // Data range:0,width
        // real width for buffer. 0 is mean that width is follow follow_type.

        // Variable name: heigh
        // Data range:
        // real heigh for buffer. 0 is mean that width is follow follow_type.

        // Variable name: follow_type
        // Data range: 0 and
        // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM,
        // 0:use configure width and height.
        // stream_type_t.use point stream width and height.

        // Variable name: follow_camera_index
        // Data range: 0 -3
        // which index camera stream width and heigh to set.
        // The value is valid when follow_type not equal to 0
        /*********************************************************************/
        {6, 0, 0, PREVIEW_STREAM, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},

    },

    // 5 hal_buffer_info     video_buffer_info
    /*ion buffer info configure. set hal layer allocate memory number,width and
       heigh*/
    {
        /*********************************************************************/
        //
        // Variable name: number
        // Data range:
        // How many number buffer to request

        // Variable name: width
        // Data range:0,width
        // real width for buffer. 0 is mean that width is follow follow_type.

        // Variable name: heigh
        // Data range:
        // real heigh for buffer. 0 is mean that width is follow follow_type.

        // Variable name: follow_type
        // Data range: 0 and
        // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM,
        // 0:use configure width and height.
        // stream_type_t.use point stream width and height.

        // Variable name: follow_camera_index
        // Data range: 0 -3
        // which index camera stream width and heigh to set.
        // The value is valid when follow_type not equal to 0
        /*********************************************************************/
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},

    },

    // 6 config_physical_descriptor    multi_phy_info
    /*struct config_physical_descriptor.camera id and stream info .*/
    {
        /*********************************************************************/
        // struct hal_stream_info.refer to configure_streams
        //
        // Variable name: type
        // Data range:
        // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
        // stream type form framework.use getStreamType function to get current
        // type

        // Variable name: width
        // Data range:
        // Corresponding to last stream type.if value is 0.use framework width

        // Variable name: heigh
        // Data range:
        // Corresponding to last stream type,if value is 0.use framework heigh

        // Variable name: format
        // Data range:
        // stream format, 0 default.

        // Variable name: follow_type
        // Data range: 0 and
        // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM,
        // 0:use configure width and height.
        // stream_type_t.use point stream width and height.
        // Variable name: follow_camera_index
        // Data range: 0 -3
        // which index camera stream width and heigh to set.
        // The value is valid when follow_type not equal to 0
        /*********************************************************************/

        /*camera device 0 physical info*/
        {
            0, // camera real id
            2, // config_stream_num
            /*stream info*/
            {
                {PREVIEW_STREAM, 0, 0, 0, 0, 0},
                {SNAPSHOT_STREAM, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

        /*camera device 1 physical info*/
        {
            2, // camera real id
            2, // config_stream_num
            /*stream info*/
            {
                {PREVIEW_STREAM, 0, 0, 0, 0, 0},
                {SNAPSHOT_STREAM, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

        /*camera device 2 physical info*/
        {
            0, // camera id
            0, // config_stream_num
            /*stream info*/
            {
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

        /*camera device 3 physical info*/
        {
            0, // camera id
            0, // config_stream_num
            /*stream info*/
            {
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

    },

    // 7 config_physical_descriptor    video_multi_phy_info
    /*struct config_physical_descriptor.camera id and stream info .*/
    {
        /*********************************************************************/
        // struct hal_stream_info.refer to configure_streams
        //
        // Variable name: type
        // Data range:
        // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
        // stream type form framework.use getStreamType function to get current
        // type

        // Variable name: width
        // Data range:
        // Corresponding to last stream type.if value is 0.use framework width

        // Variable name: heigh
        // Data range:
        // Corresponding to last stream type,if value is 0.use framework heigh

        // Variable name: format
        // Data range:
        // stream format, 0 default.

        // Variable name: follow_type
        // Data range: 0 and
        // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM,
        // 0:use configure width and height.
        // stream_type_t.use point stream width and height.
        // Variable name: follow_camera_index
        // Data range: 0 -3
        // which index camera stream width and heigh to set.
        // The value is valid when follow_type not equal to 0
        /*********************************************************************/

        /*camera device 0 physical info*/
        {
            0, // camera real id
            2, // config_stream_num
            /*stream info*/
            {
                {PREVIEW_STREAM, 0, 0, 0, 0, 0},
                {SNAPSHOT_STREAM, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

        /*camera device 1 physical info*/
        {
            2, // camera real id
            2, // config_stream_num
            /*stream info*/
            {
                {PREVIEW_STREAM, 0, 0, 0, 0, 0},
                {SNAPSHOT_STREAM, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

        /*camera device 2 physical info*/
        {
            0, // camera id
            0, // config_stream_num
            /*stream info*/
            {
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

        /*camera device 3 physical info*/
        {
            0, // camera id
            0, // config_stream_num
            /*stream info*/
            {
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
        },

    },

    // 8 configure function processCaptureRequest
    // every frame ,need to configure stream into
    // according to num_output_buffers
    /*********************************************************************/
    // How many request stream configure can to select .which configure use
    // according to mReqConfigNum. default use configure 0.
    // range 1-5,
    // ex. for first frame. for normal frame. for flush status.
    2,

    // 9 hal_req_stream_config_total    hal_req_config_stream
    {
        // configure 1
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            2,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    PREVIEW_STREAM,
                    2,
                    0,
                    {0, 1, 0, 0},
                    {PREVIEW_STREAM_FW_BUFFER, PREVIEW_STREAM_HAL_BUFFER, 0, 0},
                },

                // stream 1
                {
                    SNAPSHOT_STREAM,
                    1,
                    0,
                    {0, 0, 0, 0},
                    {SNAPSHOT_STREAM_FW_BUFFER, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 2
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            2,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    PREVIEW_STREAM,
                    2,
                    1,
                    {0, 1, 0, 0},
                    {PREVIEW_STREAM_HAL_BUFFER, PREVIEW_STREAM_FW_BUFFER, 0, 0},
                },

                // stream 1
                {
                    SNAPSHOT_STREAM,
                    1,
                    1,
                    {1, 0, 0, 0},
                    {SNAPSHOT_STREAM_FW_BUFFER, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 3
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            0,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 1
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 4
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            0,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 1
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 5
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            0,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 1
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

    },

    // 10 hal_req_stream_config_total    video_hal_req_config_stream
    {
        // configure 1
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            2,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    PREVIEW_STREAM,
                    2,
                    0,
                    {0, 1, 0, 0},
                    {PREVIEW_STREAM_FW_BUFFER, PREVIEW_STREAM_HAL_BUFFER, 0, 0},
                },

                // stream 1
                {
                    SNAPSHOT_STREAM,
                    1,
                    0,
                    {0, 0, 0, 0},
                    {SNAPSHOT_STREAM_FW_BUFFER, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 2
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            2,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    PREVIEW_STREAM,
                    2,
                    1,
                    {0, 1, 0, 0},
                    {PREVIEW_STREAM_HAL_BUFFER, PREVIEW_STREAM_FW_BUFFER, 0, 0},
                },

                // stream 1
                {
                    SNAPSHOT_STREAM,
                    1,
                    1,
                    {1, 0, 0, 0},
                    {SNAPSHOT_STREAM_FW_BUFFER, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 3
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            0,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 1
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 4
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            0,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 1
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

        // configure 5
        {
            // How many stream to configure. support max value is 5
            // stream configure info.struct hal_req_stream_config*
            0,
            {
                /*********************************************************************/
                // struct hal_req_stream_config.refer to process_capture_request
                // function
                //
                // Variable name: roi_stream_type
                // Data range:
                // DEFAULT_STREAM/PREVIEW_STREAM/VIDEO_STREAM/CALLBACK_STREAM/SNAPSHOT_STREAM
                // Original stream type form framework.use getStreamType
                // function to get current type

                // Variable name: total_camera
                // Data range: 0-3
                // How many camera to open in current stream.support max open
                // camera number is 4

                // Variable name: mn_index
                // Data range: 0-3
                // send metadate and notify camera index.

                // Variable name: camera_index
                // Data range: 0-3
                // open camera index. only camera index not camera id.

                // Variable name: stream_type_mask
                // Data range: streamTypeMask_t
                // stream mask set for every open camera. streamTypeMask_t type.
                // Support by bit or operation
                /*********************************************************************/

                // stream 0
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 1
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 2
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 3
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

                // stream 4
                {
                    0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
                },

            },
        },

    },