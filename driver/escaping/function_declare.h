
// https://docs.faircom.com/doc/sqlref/33391.htm
// https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/appendix-e-scalar-functions

// clang-format off

    // Numeric
    DECLARE2(ABS, "abs"),
    DECLARE2(ACOS, "acos"),
    DECLARE2(ASIN, "asin"),
    DECLARE2(ATAN, "atan"),
    // DECLARE2(ATAN2, ""),
    DECLARE2(CEILING, "ceil"),
    DECLARE2(COS, "cos"),
    // DECLARE2(COT, ""),
    // DECLARE2(DEGREES, ""),
    DECLARE2(EXP, "exp"),
    DECLARE2(FLOOR, "floor"),
    DECLARE2(LOG, "ln"),
    DECLARE2(LOG10, "log"),
    DECLARE2(MOD, "MOD"),
    DECLARE2(PI, "pi"),
    DECLARE2(POWER, "power"),
    // DECLARE2(RADIANS, ""),
    DECLARE2(RAND, "random"),
    DECLARE2(ROUND, "round"),
    // DECLARE2(SIGN, ""),
    DECLARE2(SIN, "sin"),
    DECLARE2(SQRT, "sqrt"),
    DECLARE2(TAN, "tan"),
    DECLARE2(TRUNCATE, "trunc"),

    // String
    // ASCII
    // BIT_LENGTH
    // CHAR
    DECLARE2(CHAR_LENGTH, "length"),
    DECLARE2(CHARACTER_LENGTH, "length"),
    DECLARE2(CONCAT, "concat"),
    // DIFFERENCE
    // INSERT
    DECLARE2(LCASE, "lower"),
    DECLARE2(LOWER, "lower"),
    // LEFT  substring(s, 0, length)
    DECLARE2(LENGTH, "length"),
    DECLARE2(LOCATE, "" /* "position" */), // special handling
    DECLARE2(CONVERT, ""), // special handling
    DECLARE2(LTRIM, ""), // special handling
    DECLARE2(OCTET_LENGTH, "length"),
    // POSITION
    // REPEAT
    DECLARE2(REPLACE, "replace"),
    // RIGHT
    // RTRIM
    // SOUNDEX
    // SPACE
    DECLARE2(SUBSTRING, "substr"),
    DECLARE2(UCASE, "upper"),
    DECLARE2(UPPER, "upper"),


    // Date
    DECLARE2(CURRENT_TIMESTAMP, ""), // special handling
    DECLARE2(CURDATE, "today"),
    DECLARE2(CURRENT_DATE, "today"),
    DECLARE2(DAYOFMONTH, "GET_DAY"),
    DECLARE2(DAYOFWEEK, "DAY_OF_WEEK"), // special handling
    DECLARE2(DAYOFYEAR, "DAY_OF_YEAR"),
    DECLARE2(EXTRACT, "EXTRACT"), // Do not touch extract inside {fn ... }
    DECLARE2(HOUR, "GET_HOUR"),
    DECLARE2(MINUTE, "GET_MINUTE"),
    DECLARE2(MONTH, "GET_MONTH"),
    DECLARE2(NOW, "now"),
    DECLARE2(SECOND, "GET_SECOND"),
    DECLARE2(TIMESTAMPADD, ""), // special handling
    DECLARE2(TIMESTAMPDIFF, ""), // special handling
    DECLARE2(WEEK, "WEEK_OF_YEAR"),
    DECLARE2(SQL_TSI_QUARTER, "QUARTER_OF_YEAR"),
    DECLARE2(YEAR, "GET_YEAR"),

    // DECLARE2(DATABASE, ""),
    DECLARE2(IFNULL, "ifNull"),
    // DECLARE2(USER, ""),