/**
 * (C) 2007-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * cellinfo_builder.cpp for ...
 *
 * Authors:
 *   yubai <yubai.lk@taobao.com>
 *
 */
#include "common/ob_crc64.h"
#include "updateserver/ob_ups_utils.h"
#include "cellinfo_builder.h"
#include "utils.h"

using namespace oceanbase;
using namespace common;
using namespace updateserver;

CellinfoBuilder::CellinfoBuilder(const int64_t max_op_num) : max_op_num_(max_op_num)
{
}

CellinfoBuilder::~CellinfoBuilder()
{
}

int CellinfoBuilder::check_schema_mgr(const ObSchemaManager &schema_mgr)
{
  int ret = OB_SUCCESS;
  const ObSchema *schema = NULL;
  for (schema = schema_mgr.begin(); schema != schema_mgr.end(); ++schema)
  {
    if (NULL == schema->find_column_info(SEED_COLUMN_NAME)
        || NULL == schema->find_column_info(ROWKEY_INFO_COLUMN_NAME)
        || SEED_COLUMN_ID != schema->find_column_info(SEED_COLUMN_NAME)->get_id()
        || ROWKEY_INFO_COLUMN_ID != schema->find_column_info(ROWKEY_INFO_COLUMN_NAME)->get_id())
    {
      TBSYS_LOG(WARN, "no seed or rowkey_info column in the schema");
      ret = OB_ERROR;
      break;
    }
  }
  return ret;
}

int CellinfoBuilder::get_mutator(const ObString &row_key, const ObSchema &schema, int64_t &cur_seed,
                                ObMutator &mutator, PageArena<char> &allocer)
{
  int ret = OB_SUCCESS;
  
  int64_t sign = ob_crc64(row_key.ptr(), row_key.length());
  uint64_t table_id = schema.get_table_id();
  sign = ob_crc64(sign, &table_id, sizeof(uint64_t));

  struct drand48_data rand_data;
  cur_seed += 1;
  srand48_r(sign + cur_seed, &rand_data);

  int64_t rand = 0;
  lrand48_r(&rand_data, &rand);
  
  int64_t op_num = range_rand(1, max_op_num_, rand);
  for (int64_t i = 0; i < op_num; i++)
  {
    ret = build_operator_(rand_data, row_key, schema, mutator, allocer);
    if (OB_SUCCESS != ret)
    {
      break;
    }
  }
  ObString table_name;
  table_name.assign_ptr(const_cast<char*>(schema.get_table_name()), strlen(schema.get_table_name()));
  ObString column_name;
  column_name.assign_ptr(SEED_COLUMN_NAME, strlen(SEED_COLUMN_NAME));
  ObObj obj;
  obj.set_int(cur_seed);
  mutator.update(table_name, row_key, column_name, obj);

  return ret;
}

int CellinfoBuilder::calc_op_type_(const int64_t rand)
{
  int op_type = 0;
  //int op_range = range_rand(1, 100, rand);
  //if (10 >= op_range)
  //{
  //  op_type = DEL_ROW;
  //}
  //else if (20 >= op_range)
  //{
  //  op_type = DEL_CELL;
  //}
  //else if (40 >= op_range)
  //{
    op_type = UPDATE;
  //}
  //else if (60 >= op_range)
  //{
  //  op_type = INSERT;
  //}
  //else
  //{
  //  op_type = ADD;
  //}
  return op_type;
}

int CellinfoBuilder::build_operator_(struct drand48_data &rand_data, const ObString &row_key, const ObSchema &schema,
                                    ObMutator &mutator, PageArena<char> &allocer)
{
  int ret = OB_SUCCESS;

  int op_type = 0;
  int64_t column_pos = 0;
  ObObj obj;
  build_cell_(rand_data, row_key, schema,
              obj, column_pos, op_type,
              allocer);
  const ObColumnSchema *column_schema = schema.column_begin();

  ObString table_name;
  ObString column_name;
  table_name.assign_ptr(const_cast<char*>(schema.get_table_name()), strlen(schema.get_table_name()));
  column_name.assign_ptr(const_cast<char*>(column_schema[column_pos].get_name()),
                        strlen(column_schema[column_pos].get_name()));
  switch (op_type)
  {
    case DEL_CELL:
    case UPDATE:
    case ADD:
      mutator.update(table_name, row_key, column_name, obj);
      break;
    case INSERT:
      mutator.insert(table_name, row_key, column_name, obj);
      break;
    case DEL_ROW:
      ret = mutator.del_row(table_name, row_key);
      break;
    default:
      break;
  }

  return ret;
}

void CellinfoBuilder::build_cell_(struct drand48_data &rand_data, const ObString &row_key, const ObSchema &schema,
                                  ObObj &obj, int64_t &column_pos, int &op_type,
                                  PageArena<char> &allocer)
{
  const ObColumnSchema *column_schema = schema.column_begin();
  int64_t column_num = schema.column_end() - schema.column_begin();
  int64_t rand = 0;

  lrand48_r(&rand_data, &rand);
  op_type = calc_op_type_(rand);
  //while (true)
  //{
  //  lrand48_r(&rand_data, &rand);
  //  column_pos = range_rand(0, column_num - 3, rand);
  //  if (ObIntType <= column_schema[column_pos].get_type()
  //      && ObVarcharType >= column_schema[column_pos].get_type())
  //  {
  //    break;
  //  }
  //}
  column_pos=0;

  lrand48_r(&rand_data, &rand);
  switch (column_schema[column_pos].get_type())
  {
    case ObIntType:
      {
        int64_t tmp = rand;
        obj.set_int(tmp, ADD == op_type);
        break;
      }
    case ObFloatType:
      {
        float tmp = static_cast<float>(rand);
        obj.set_float(tmp, ADD == op_type);
        break;
      }
    case ObDoubleType:
      {
        double tmp = static_cast<double>(rand);
        obj.set_double(tmp, ADD == op_type);
        break;
      }
    case ObDateTimeType:
      {
        ObDateTime tmp = static_cast<ObDateTime>(rand);
        obj.set_datetime(tmp, ADD == op_type);
        break;
      }
    case ObPreciseDateTimeType:
      {
        ObPreciseDateTime tmp = static_cast<ObPreciseDateTime>(rand);
        obj.set_precise_datetime(tmp, ADD == op_type);
        break;
      }
    case ObVarcharType:
      {
        int64_t length = range_rand(1, column_schema[column_pos].get_size(), rand);
        char *ptr = allocer.alloc(length);
        build_string(ptr, length, rand);
        ObString str;
        str.assign_ptr(ptr, length);
        if (ADD == op_type)
        {
          op_type = UPDATE;
        }
        obj.set_varchar(str);
        break;
      }
    default:
      break;
  }
  if (DEL_CELL == op_type)
  {
    obj.set_null();
  }
  else if (DEL_ROW == op_type)
  {
    obj.set_ext(ObActionFlag::OP_DEL_ROW);
  }
}

int CellinfoBuilder::get_result(const ObString &row_key, const ObSchema &schema,
                                const int64_t begin_seed, const int64_t end_seed,
                                result_set_t &result, PageArena<char> &allocer)
{
  int ret = OB_SUCCESS;
  for (int64_t seed = begin_seed; seed <= end_seed; seed++)
  {
    int64_t sign = ob_crc64(row_key.ptr(), row_key.length());
    uint64_t table_id = schema.get_table_id();
    sign = ob_crc64(sign, &table_id, sizeof(uint64_t));

    struct drand48_data rand_data;
    srand48_r(sign + seed, &rand_data);

    int64_t rand = 0;
    lrand48_r(&rand_data, &rand);

    int64_t op_num = range_rand(1, max_op_num_, rand);
    for (int64_t i = 0; i < op_num; i++)
    {
      int op_type = 0;
      int64_t column_pos = 0;
      ObCellInfo *cell_info = (ObCellInfo*)allocer.alloc(sizeof(ObCellInfo));
      cell_info->reset();
      const ObColumnSchema *column_schema = schema.column_begin();

      build_cell_(rand_data, row_key, schema,
                  cell_info->value_, column_pos, op_type,
                  allocer);
      cell_info->table_id_ = table_id;
      cell_info->row_key_ = row_key;
      if (DEL_ROW == op_type)
      {
        cell_info->column_id_ = OB_INVALID_ID;
      }
      else
      {
        cell_info->column_id_ = column_schema[column_pos].get_id();
      }

      merge_obj(op_type, cell_info, result);
    }
  }
  return ret;
}

void CellinfoBuilder::merge_obj(const int64_t op_type, ObCellInfo *cell_info, result_set_t &result)
{
  if (ObExtendType == cell_info->value_.get_type()
      && ObActionFlag::OP_DEL_ROW == cell_info->value_.get_ext())
  {
    result.clear();
    cell_info->column_id_ = OB_INVALID_ID;
  }
  ObCellInfo *ret_ci = NULL;
  int hash_ret = result.get(cell_info->column_id_, ret_ci);
  if (HASH_EXIST == hash_ret)
  {
    if (OB_SUCCESS != ret_ci->value_.apply(cell_info->value_))
    {
      TBSYS_LOG(WARN, "apply obj fail [%s] [%s]",
                print_obj(ret_ci->value_), print_obj(cell_info->value_));
    }
  }
  else
  {
    if (HASH_INSERT_SUCC != result.set(cell_info->column_id_, cell_info, 0))
    {
      TBSYS_LOG(WARN, "set to result hash fail column_id=%lu [%s]",
                cell_info->column_id_, print_obj(cell_info->value_));
    }
  }
}


