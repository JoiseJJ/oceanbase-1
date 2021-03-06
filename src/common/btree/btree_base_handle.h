/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ./btree_base_handle.h for ...
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *
 */
#ifndef OCEANBASE_COMMON_BTREE_BTREE_BASE_HANDLE_H_
#define OCEANBASE_COMMON_BTREE_BTREE_BASE_HANDLE_H_

#include <stdint.h>

namespace oceanbase
{
  namespace common
  {
    /**
     * 用于读的时候用
     */
    class BtreeRootPointer;
    class BtreeBaseHandle
    {
      friend class BtreeBase;
    public:
      /**
       * 构造
       */
      BtreeBaseHandle();
      /**
       * 析构
       */
      virtual ~BtreeBaseHandle();

      /**
       * root_pointer is null
       */
      bool is_null_pointer();
      /**
       * 清理,把引用计数减1
       */
      void clear();

    protected:
      // root指针
      BtreeRootPointer *root_pointer_;
      volatile int64_t *ref_count_;
    };

    /**
     * 用于回调用
     */
    class BtreeCallback
    {
    public:
      virtual ~BtreeCallback();
      virtual int callback_done(void *data) = 0;
    };
  } // end namespace common
} // end namespace oceanbase

#endif


