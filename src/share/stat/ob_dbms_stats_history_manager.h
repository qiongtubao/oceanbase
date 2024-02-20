/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OB_DBMS_STATS_HISTORY_MANAGER_H
#define OB_DBMS_STATS_HISTORY_MANAGER_H

#include "share/stat/ob_stat_define.h"
#include "sql/engine/ob_exec_context.h"
#include "share/stat/ob_stat_item.h"
#include "share/stat/ob_opt_table_stat.h"
#include "share/stat/ob_opt_column_stat_cache.h"

namespace oceanbase {
using namespace sql;
namespace common {

struct ObOptTableStatHandle;
static int64_t MAX_HISTORY_RETENTION = 365001;
static int64_t BATCH_DELETE_MAX_ROWCNT = 10000;
static int64_t BATCH_DELETE_MAX_QUERY_TIMEOUT = 43200000000;//12 * 60 * 60 * 1000000LL
enum ObOptStatsDeleteFlags
{
  DELETE_NONE                = 0,
  DELETE_TAB_STAT_HISTORY    = 1,
  DELETE_COL_STAT_HISTORY    = 1 << 1,
  DELETE_HIST_STAT_HISTORY   = 1 << 2,
  DELETE_TASK_GATHER_HISTORY = 1 << 3,
  DELETE_TAB_GATHER_HISTORY  = 1 << 4,
  DELETE_USELESS_COL_STAT    = 1 << 5,
  DELETE_USELESS_HIST_STAT   = 1 << 6,
  DELETE_ALL                 = (1 << 7) - 1
};
class ObDbmsStatsHistoryManager
{
public:
  static int get_history_stat_handles(ObExecContext &ctx,
                                      const ObTableStatParam &param,
                                      ObIArray<ObOptTableStatHandle> &history_tab_handles,
                                      ObIArray<ObOptColumnStatHandle> &history_col_handles);

  static int restore_table_stats(ObExecContext &ctx,
                                 const ObTableStatParam &param,
                                 const int64_t specify_time);

  static int purge_stats(ObExecContext &ctx, const int64_t specify_time);

  static int get_stats_history_retention_and_availability(ObExecContext &ctx,
                                                          bool fetch_history_retention,
                                                          ObObj &result);

  static int alter_stats_history_retention(ObExecContext &ctx, const int64_t new_retention);

private:

  static int fetch_table_stat_histrory(ObExecContext &ctx,
                                       const ObTableStatParam &param,
                                       const int64_t specify_time,
                                       ObIArray<ObOptTableStat*> &all_part_stats);

  static int fill_table_stat_history(ObIAllocator &allocator,
                                     common::sqlclient::ObMySQLResult &result,
                                     ObOptTableStat *&stat);

  static int fetch_column_stat_history(ObExecContext &ctx,
                                       const ObTableStatParam &param,
                                       const int64_t specify_time,
                                       ObIArray<ObOptColumnStat*> &all_cstats);

  static int fill_column_stat_history(ObIAllocator &allocator,
                                      common::sqlclient::ObMySQLResult &result,
                                      ObOptColumnStat *&col_stat);

  static int fetch_histogram_stat_histroy(ObExecContext &ctx,
                                          ObIAllocator &allocator,
                                          const int64_t specify_time,
                                          ObOptColumnStat &col_stat);

  static int fill_bucket_stat_histroy(ObIAllocator &allocator,
                                      sqlclient::ObMySQLResult &result,
                                      ObOptColumnStat &stat);

  static int get_stats_history_retention(ObExecContext &ctx, int64_t &retention_val);

  static int set_col_stat_cs_type(const ObIArray<ObColumnStatParam> &column_params,
                                  ObIArray<ObOptColumnStatHandle> &col_handles);

  static int set_col_stat_cs_type(const ObIArray<ObColumnStatParam> &column_params,
                                  ObOptColumnStat *&col_stat);

  static int gen_partition_list(const ObTableStatParam &param,
                                ObSqlString &partition_list);

  static int remove_useless_column_stats(ObMySQLTransaction &trans,
                                         uint64_t tenant_id,
                                         const uint64_t start_time,
                                         const uint64_t max_duration_time,
                                         int64_t &delete_flags);

  static int do_delete_expired_stat_history(ObMySQLTransaction &trans,
                                            const uint64_t tenant_id,
                                            const uint64_t start_time,
                                            const uint64_t max_duration_time,
                                            const char* specify_time_str,
                                            const char* process_table_name,
                                            ObOptStatsDeleteFlags cur_delete_flag,
                                            int64_t &delete_flags);
};


} // end of sql
} // end of namespace

#endif //OB_DBMS_STATS_HISTORY_MANAGER_H
