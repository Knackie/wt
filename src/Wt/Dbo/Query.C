/*
 * Copyright (C) 2010 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#include "Query"
#include "Query_impl.h"

#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <stdexcept>

namespace Wt {
  namespace Dbo {
    namespace Impl {

ParameterBase::~ParameterBase()
{ }

std::string createQuerySql(StatementKind kind,
			   const std::vector<FieldInfo>& fields,
			   const std::string& from)
{
  if (kind == Select)
    return "select " + selectColumns(fields) + ' ' + from;
  else {
    /*
     * We cannot have " order by " in our query, but we cannot simply
     * junk evertything after " order by " since there may still be
     * parameters referenced in the " limit " or " offset " clause.
     */
    std::string result = "select count(1) " + from;

    std::size_t o = result.find(" order by ");
    if (o != std::string::npos) {
      std::size_t l = result.find(" limit ");
      std::size_t f = result.find(" offset ");

      std::size_t len;
      if (l != std::string::npos)
	len = l - o;
      else if (f != std::string::npos)
	len = f - o;
      else
	len = std::string::npos;

      result.erase(o, len);
    }

    return result;
  }
}

std::string createQuerySql(StatementKind kind,
			   const std::vector<FieldInfo>& fields,
			   const std::string& from,
			   const std::string& where,
			   const std::string& groupBy,
			   const std::string& orderBy,
			   int limit, int offset)
{
  std::string result;

  if (kind == Select) {
    result = "select " + selectColumns(fields);
  } else if (kind == Count) {
    /*
     * If there is a group by, then we cannot simply substitute count(*)
     * (at least that still gives multiple results for sqlite3?
     */
    if (!groupBy.empty()) {
      return "select count(1) from ("
	+ createQuerySql(Select, fields,
			 from, where, groupBy, orderBy, limit, offset)
	+ ")";
    } else
      result = "select count(1)";
  }

  result += ' ' + from;
      
  if (!where.empty())
    result += " where " + where;

  if (!groupBy.empty()) {
    std::vector<std::string> groupByFields;
    boost::split(groupByFields, groupBy, boost::is_any_of(","));

    for (unsigned i = 0; i < groupByFields.size(); ++i) {
      boost::trim(groupByFields[i]);

      std::string g;
      for (unsigned j = 0; j < fields.size(); ++j)
	if (fields[j].qualifier() == groupByFields[i]) {
	  if (!g.empty())
	    g += ", ";
	  g += fields[j].sql();
	}

      if (!g.empty())
	groupByFields[i] = g;
    }

    result += " group by ";
    for (unsigned i = 0; i < groupByFields.size(); ++i) {
      if (i != 0)
	result += ", ";
      result += groupByFields[i];
    }
  }

  if (kind == Select && !orderBy.empty())
    result += " order by " + orderBy;

  if (limit != -1)
    result += " limit ?";

  if (offset != -1)
    result += " offset ?";

  return result;
}

void parseSql(const std::string& sql,
	      std::vector<std::string>& aliases,
	      std::string& rest)
{
  std::size_t selectPos = sql.find("select ");
  if (selectPos == std::string::npos) {
    aliases.clear();
    rest = sql;
    return;
  }

  if (selectPos != 0)
    throw std::logic_error("Session::query(): query should start with 'select '"
			   " (sql='" + sql + "'");

  std::string aliasStr;
  std::size_t fromPos = sql.find(" from ");
  if (fromPos != std::string::npos) {
    aliasStr = sql.substr(7, fromPos - 7);
    rest = sql.substr(fromPos);
  } else {
    aliasStr = sql.substr(7);
    rest = std::string();
  }

  boost::split(aliases, aliasStr, boost::is_any_of(","));
  for (unsigned i = 0; i < aliases.size(); ++i)
    boost::trim(aliases[i]);
}

std::string selectColumns(const std::vector<FieldInfo>& fields) {
  std::string result;

  for (unsigned i = 0; i < fields.size(); ++i) {
    const FieldInfo& field = fields[i];
    if (!result.empty())
      result += ", ";

    result += field.sql();
  }

  return result;
}

    }
  }
}
				 