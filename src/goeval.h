#ifndef GOEVAL_H
#define GOEVAL_H

struct analyzer_id {
	std::string engine;
	double komi = 0;
	bool komi_set = false;

	bool operator== (const analyzer_id &other) const
	{
		return engine == other.engine && komi_set == other.komi_set && (!komi_set || komi == other.komi);
	}
	bool operator!= (const analyzer_id &other) const
	{
		return !(*this == other);
	}
};

struct eval {
	int visits = 0;
	/* The score is stored from Black's perspective, as is the winrate.  */
	double score_mean = 0;
	double score_stddev = 0;
	double wr_black = 0.5;
	analyzer_id id;
};

#endif
