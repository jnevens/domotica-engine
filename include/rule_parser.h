/*
 * rules_parser.h
 *
 *  Created on: Dec 18, 2016
 *      Author: jnevens
 */

#ifndef RULE_PARSER_H_
#define RULE_PARSER_H_

int rules_read_dir(const char *dir, int pass);
int rules_read_file(const char *file_path, int pass);

#endif /* RULE_PARSER_H_ */
