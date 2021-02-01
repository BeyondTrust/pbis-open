BEGIN   {
            in_enterprise_section = 0;
        }

/^\[ENTERPRISE\]/ {
                        in_enterprise_section = 1;
                        getline
                    }
/^\[\/ENTERPRISE\]/   {
                        in_enterprise_section = 0;
                        getline
                    }

                    {
                        if (in_enterprise_section)
                        {
                            if (enterprise)
                            {
                                print $0;
                            }
                        }
                        else
                        {
                            print $0;
                        }
                    }

